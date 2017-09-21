#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <ctype.h>

#define WIDTH 21924
#define HEIGHT 10967

void output(unsigned char *image, int width, int height, int depth, char *filename,
	    int left, int top, int right, int bottom) {
	int quality = 85;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *outfile;		 /* target file */
	JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
	int row_stride;		 /* physical row width in image buffer */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = right - left; /* image width and height, in pixels */
	cinfo.image_height = bottom - top;
	cinfo.input_components = depth; /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = (right - left) * depth; /* JSAMPLEs per row in image_buffer */

	int y = top;
	while (cinfo.next_scanline < cinfo.image_height) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
		row_pointer[0] = &image[y * depth * width + left * depth];
		//row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		y++;
	}

	/* Step 6: Finish compression */

	jpeg_finish_compress(&cinfo);
	/* After finish_compress, we can close the output file. */
	fclose(outfile);

	/* Step 7: release JPEG compression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress(&cinfo);

	/* And we're done! */
}

void read_jpeg(char *filename, double magnitude, unsigned char *img) {
	struct jpeg_decompress_struct cinfo;
	FILE *infile;      /* source file */
	JSAMPARRAY buffer; /* Output row buffer */
	int row_stride;    /* physical row width in output buffer */

	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return;
	}

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);

	(void) jpeg_read_header(&cinfo, TRUE);
	(void) jpeg_start_decompress(&cinfo);

	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	int y = 0;

	while (cinfo.output_scanline < cinfo.output_height) {
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);

		if (y < HEIGHT) {
			int x;
			for (x = 0; x < cinfo.output_width * 3 && x < WIDTH * 3; x++) {
				img[y * WIDTH * 3 + x] += buffer[0][x] * magnitude;
			}
		}

		y++;
	}

	(void) jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
}

int main(int argc, char **argv) {
	unsigned char *img = malloc(WIDTH * HEIGHT * 3);
	memset(img, 0, WIDTH * HEIGHT * 3);

	read_jpeg("../world-landor.jpg", .9, img);
	read_jpeg("../world-base-big.jpg", .1, img);

	output(img, WIDTH, HEIGHT, 3, "out.jpg", 0, 0, WIDTH, HEIGHT);
}
