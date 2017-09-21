#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <ctype.h>

#define ENOUGH 125
#define MINIMUM 200

#define COLOR 3

char *num(char *start, int *out) {
	for (; *start; start++) {
		if (isdigit(*start)) {
			break;
		}
	}

	while (*start == '0') {
		start++;
	}

	*out = atoi(start);

	for (; *start; start++) {
		if (!isdigit(*start)) {
			break;
		}
	}

	return start;
}

int oseries = 0, oindex = 0;
int osub = 0;

void output(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom) {
  char fname[100];
  int quality = 85;

	sprintf(fname, "crop/%s", filename);
	char *jpg = strstr(fname, ".jpg");

	if (jpg == NULL) {
		fprintf(stderr, "no .jpg in %s\n", fname);
		return;
	}

	sprintf(jpg, "%c.jpg", osub + 'a');

	osub++;
printf("write %s\n", fname);

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * outfile;		/* target file */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  if ((outfile = fopen(fname, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", fname);
    exit(1);
  }
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = right - left; 	/* image width and height, in pixels */
  cinfo.image_height = bottom - top;
  cinfo.input_components = depth;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  jpeg_start_compress(&cinfo, TRUE);

  row_stride = (right - left) * depth;	/* JSAMPLEs per row in image_buffer */

  int y = top;
  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & image[y * depth * width + left * depth];
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

void processh(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom, int stable) {
	int starts[width];
	int ends[width];
	int resultcount = 0;

	stable++;

	int x;

	for (x = left * depth; x < right * depth; x += 3) {
		int count = 0;
		int y;

		for (y = top; y < bottom; y++) {
			int where = y * depth * width + x;
			int red = image[where];
			int green = image[where + 1];
			int blue = image[where + 2];
			int darkness = (red) + (green) + (blue);
			int difference = abs(red - green) + abs(green - blue) + abs(red - blue);
			int badness = darkness + COLOR * difference;

			count += badness;
		}

		//printf("found horiz 1n badness %d vs %d\n", count, (int) ((bottom - top) * ENOUGH));
		if (count >= (bottom - top) * ENOUGH) {
			int xstart = x;

			for (; x < right * depth; x += 3) {
				count = 0;

				for (y = top; y < bottom; y++) {
					int where = y * depth * width + x;
					int red = image[where];
					int green = image[where + 1];
					int blue = image[where + 2];
					int darkness = (red) + (green) + (blue);
					int difference = abs(red - green) + abs(green - blue) + abs(red - blue);
					int badness = darkness + COLOR * difference;

					count += badness;
				}

		//printf("found horiz 2 badness %d vs %d\n", count, (int) ((bottom - top) * ENOUGH));
				if (count < (bottom - top) * ENOUGH) {
					break;
				}
			}

			int xend = x;

			xstart = xstart / depth;
			xend = xend / depth;

			if (xend > xstart + MINIMUM) {
				starts[resultcount] = xstart;
				ends[resultcount] = xend;
				resultcount++;
				printf("x range %d to %d\n", xstart, xend);
			}
		}
	}

	if (starts[0] != left && ends[0] != right) {
		stable = 0;
	}

	if (resultcount == 1 && stable > 2) {
		printf("-- exactly %d %d %d %d\n", starts[0], top, ends[0], bottom);
		output(image, width, height, depth, filename,
			 starts[0], top, ends[0], bottom);
	} else {
		int i;

		for (i = 0; i < resultcount; i++) {
			process(image, width, height, depth, filename,
				 starts[i], top, ends[i], bottom, stable);

		}
	}
}

void process(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom, int stable) {
	printf("processing %d by %d, depth %d\n", width, height, depth);
	int y;

	stable++;

	for (y = top; y < bottom; y++) {
		int count = 0;
		int x;

		for (x = left * depth; x < right * depth; x += 3) {
			int where = y * depth * width + x;
			int red = image[where];
			int green = image[where + 1];
			int blue = image[where + 2];
			int darkness = (red) + (green) + (blue);
			int difference = abs(red - green) + abs(green - blue) + abs(red - blue);
			int badness = darkness + COLOR * difference;

			count += badness;
		}

		//printf("found vertical 1 badness %d vs %d\n", count, (int) ((right - left) * ENOUGH));
		if (count >= (right - left) * ENOUGH) {
			int ystart = y;

			for (; y < bottom; y++) {
				count = 0;

				for (x = left * depth; x < right * depth; x += 3) {
					int where = y * depth * width + x;
					int red = image[where];
					int green = image[where + 1];
					int blue = image[where + 2];
					int darkness = (red) + (green) + (blue);
					int difference = abs(red - green) + abs(green - blue) + abs(red - blue);
					int badness = darkness + COLOR * difference;

					count += badness;
				}

		//printf("found vertical 2 badness %d vs %d\n", count, (int) ((right - left) * ENOUGH));

				if (count < (right - left) * ENOUGH) {
					break;
				}
			}

			int yend = y;

			if (yend > ystart + MINIMUM) {
				printf("y range %d to %d\n", ystart, yend);

				if (ystart != top || yend != bottom) {
					stable = 0;
				}

				processh(image, width, height, depth, filename,
					 left, ystart, right, yend, stable);
			}
		}
	}
}

void read_JPEG_file(char *filename)
{
  struct jpeg_decompress_struct cinfo;
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */

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
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

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
		 0, 0, cinfo.output_width, cinfo.output_height, 0);
  free(image);

  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

}

int main(int argc, char **argv) {
	int i;

	for (i = 1; i < argc; i++) {
		osub = 0;
		read_JPEG_file(argv[i]);
	}
}
