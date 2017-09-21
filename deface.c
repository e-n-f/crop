#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <ctype.h>
#include <math.h>

void output(unsigned char *image, int width, int height, int depth, char *filename,
	    int left, int top, int right, int bottom) {
	static char oldfile[1000] = "";

	char fname[50];
	int quality = 75;
	static int count = 0;

	printf("%s vs %s %d\n", oldfile, filename, strcmp(filename, oldfile));

	char *base = filename;
	char *cp;
	for (cp = filename; *cp; cp++) {
		if (*cp == '/') {
			base = cp + 1;
		}
	}

	//sprintf(fname, "out/%04d.jpg", count++);
	if (strcmp(filename, oldfile) == 0) {
		char tmpfile[1000];
		strcpy(tmpfile, base);

		for (cp = tmpfile; *cp; cp++) {
			if (*cp == '.') {
				*cp = '\0';
				break;
			}
		}

		count++;
		sprintf(fname, "out/%s-%d.jpg", tmpfile, count);
	} else {
		sprintf(fname, "out/%s", base);
		count = 0;
	}

	strcpy(oldfile, filename);

	printf("write %s\n", fname);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *outfile;		 /* target file */
	JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
	int row_stride;		 /* physical row width in image buffer */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(fname, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", fname);
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

float dist(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;

	return sqrt((dx * dx) + (dy * dy));
}

void process(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom, float x1, float y1, float x2, float y2, float x3, float y3) {
	// printf("processing %d by %d, depth %d\n", width, height, depth);

	// printf("%f,%f %f,%f %f,%f\n", x1, y1, x2, y2, x3, y3);

	float deye = dist(x1, y1, x2, y2);
	float dleft = dist(x1, y1, x3, y3);
	float dright = dist(x2, y2, x3, y3);

	printf("ratio %.4f %.4f\n", dleft / deye, dright / deye);

	x2 = (x1 + x2) / 2;

	if (x3 == x2) {
		x3 += .0001;
	}
	if (y3 == y2) {
		y3 += .0001;
	}

	float dx = (x3 - x2);
	float dy = (y3 - y2);
	float dist = sqrt((dx * dx) + (dy * dy));
	//float ang = atan2(dy, dx);
	float ang = atan2(1.7, 0);
	printf("angle is %f for %f,%f to %f,%f\n", ang * 180 / M_PI, x2, y2, x3, y3);

	float odist = 37;
	float oang = atan2(131 - 94, 0);

	int nwidth, nheight;

	nwidth = 200;
	nheight = 284;

	// printf("scale to %d by %d\n", nwidth, nheight);
	unsigned char outbuf[284 * 200 * depth];

	int x, y, layer;
	for (y = 0; y < nheight; y++) {
		for (x = 0; x < nwidth; x++) {
			// unit vector is 90, 94 to 90, 131

			// shift

			float nx = x - 94;
			float ny = y - 90;

			// normalize

			nx = nx / odist;
			ny = ny / odist;

			// convert to polar

			float d = sqrt((nx * nx) + (ny * ny));
			float a = atan2(ny, nx);

			// derotate

			a -= oang;

			// rotate

			a += ang;

			// convert from polar;

			float origx = d * cos(a);
			float origy = d * sin(a);

			// denormalize

			origx *= dist;
			origy *= dist;

			// deshift

			origx += x2;
			origy += y2;

#if 0
			float ny = (y - 94) / 37;
			float nx = (x - 90) / 37;

			float origx = dist * nx * cos(ang) + x2;
			float origy = dist * ny * sin(ang) + y2;
#endif

// printf("%d,%d to %f,%f\n", x, y, origx, origy);

#if 0
			int origx = x * width / nwidth;
			int origy = y * height / nheight;
#endif

			if (origx < 0 || origx >= width) {
				origx = 0;
				origy = 0;
			}
			if (origy < 0 || origy >= height) {
				origx = 0;
				origy = 0;
			}

			int origoff = (int) origx * depth +
				      (int) origy * width * depth;

			int off = x * depth + y * nwidth * depth;

			for (layer = 0; layer < depth; layer++) {
				outbuf[off + layer] = image[origoff + layer];
			}
		}
	}

	output(outbuf, nwidth, nheight, depth, filename, 0, 0, nwidth, nheight);

	return;
}

void read_JPEG_file(char *filename, float x1, float y1, float x2, float y2, float x3, float y3) {
	struct jpeg_decompress_struct cinfo;
	FILE *infile;      /* source file */
	JSAMPARRAY buffer; /* Output row buffer */
	int row_stride;    /* physical row width in output buffer */

	unsigned char *image;
	unsigned char *where;

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

	image = malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
	if (image == NULL) {
		fprintf(stderr, "failed to allocate memory\n");
		exit(EXIT_FAILURE);
	}
	where = image;

	while (cinfo.output_scanline < cinfo.output_height) {
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);

		memcpy(where, buffer[0], row_stride);
		where += row_stride;
	}

	(void) jpeg_finish_decompress(&cinfo);

	process(image, cinfo.output_width, cinfo.output_height, cinfo.output_components, filename,
		0, 0, cinfo.output_width, cinfo.output_height, x1, y1, x2, y2, x3, y3);
	free(image);

	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
}

int main(int argc, char **argv) {
	char s[2000];
	char filename[600];
	float x1, y1, x2, y2, x3, y3, conf;

	while (fgets(s, 2000, stdin)) {
		sscanf(s, "%s %f,%f and %f,%f mouth %f,%f confidence %f\n", filename,
		       &x1, &y1, &x2, &y2, &x3, &y3, &conf);
		printf("%s -> %f %f %f %f %f %f\n", filename, x1, y1, x2, y2, x3, y3);

		read_JPEG_file(filename, x1, y1, x2, y2, x3, y3);
	}
}
