import numba
import numpy as np
import sys
import cv2
import time
from numba import cuda, float32, uint32, int32

@cuda.jit(device=True)
def get_pixel(img, shift, x, y):
	if x >= 0 and x < img.shape[1] and y >= 0 and y < img.shape[0]:
		return uint32(img[y, x, shift])
	return uint32(0)

@cuda.jit()
def cuda_gaussian_filter(cuda_input_image, cuda_sum, shift, sx, sy, cuda_filter_G):
	img_row = cuda.shared.array((16, 64), dtype=uint32)
	filter_row = cuda.shared.array((8, 32), dtype=uint32)

	img_width = cuda_input_image.shape[1]
	img_height = cuda_input_image.shape[0]
	ws = cuda_filter_G.shape[0]

	x, y = cuda.grid(2)
	x += sx - ws//2
	y += sy - ws//2
	tx = int32(cuda.threadIdx.x)
	ty = int32(cuda.threadIdx.y)

	img_row[ty, tx] = get_pixel(cuda_input_image, shift, x, y)
	img_row[ty, tx+32] = get_pixel(cuda_input_image, shift, x+32, y)
	img_row[ty+8, tx] = get_pixel(cuda_input_image, shift, x, y+8)
	img_row[ty+8, tx+32] = get_pixel(cuda_input_image, shift, x+32, y+8)

	filter_row[ty, tx] = cuda_filter_G[ty+sy, tx+sx] if ty+sy < ws and tx+sx < ws else 0
	cuda.syncthreads()

	x, y = cuda.grid(2)
	if x >= img_width or y >= img_height:
		return

	a = cuda_sum[y, x]
	for i in range(8):
		for j in range(32):
			a += img_row[i+ty, j+tx] * filter_row[i, j]
	cuda_sum[y, x] = a

@cuda.jit()
def cuda_output_sum(cuda_output_image, cuda_sum, shift, FILTER_SCALE):
	x, y = cuda.grid(2)
	img_width = cuda_sum.shape[1]
	img_height = cuda_sum.shape[0]
	if x < img_width and y < img_height:
		tmp = cuda_sum[y, x] // FILTER_SCALE
		cuda_output_image[y, x, shift] = min(255, tmp)
		cuda_sum[y, x] = 0

with open("mask_Gaussian.txt") as f:
	FILTER_SIZE = int(f.readline())
	ws = int(np.sqrt(FILTER_SIZE))
	assert(ws * ws == FILTER_SIZE)
	filter_G = np.zeros((ws,ws), dtype=np.uint32)
	for i in range(ws):
		row = f.readline().rstrip().split(' ')
		for j in range(ws):
			filter_G[i,j] = int(row[j])
	FILTER_SCALE = uint32(np.sum(filter_G))

cuda.select_device(0)
print("Device: %s" % cuda.gpus.current.name)

filename = sys.argv[1]
input_image = cv2.imread(filename)
img_height, img_width, img_channel = np.shape(input_image)
print("Filter scale = %d, filter size %d x %d and image size W = %d, H = %d" % (FILTER_SCALE, ws, ws, img_width, img_height));

output_image = np.zeros(input_image.shape, dtype=np.uint8)

cuda_input_image = cuda.to_device(input_image)
cuda_filter_G = cuda.to_device(filter_G)
cuda_output_image = cuda.device_array(output_image.shape, np.uint8)
cuda_sum = cuda.device_array((img_height, img_width), np.uint32)

block_size = (32, 8)
grid_size = ((img_width+31) // 32, (img_height+7) // 8)
for shift in range(img_channel):
	for sx in range(0, ws, 32):
		for sy in range(0, ws, 8):
			cuda_gaussian_filter[grid_size, block_size](
				cuda_input_image, cuda_sum, int32(shift), int32(sx), int32(sy), cuda_filter_G
			)
	cuda_output_sum[grid_size, block_size](
		cuda_output_image, cuda_sum, shift, FILTER_SCALE
	)
cuda_output_image.copy_to_host(output_image)

cv2.imshow("python gaussian blur", output_image)
cv2.waitKey(16)
time.sleep(1)
