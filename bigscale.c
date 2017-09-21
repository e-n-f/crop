#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <ctype.h>

void output(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom) {
  char fname[50];
  int quality = 60;
	static int count = 0;

  sprintf(fname, "out/%04d.jpg", count++);
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

void process(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom) {
	printf("processing %d by %d, depth %d\n", width, height, depth);

	int nwidth, nheight;

	nwidth = 2000;
	nheight = height * 2000 / width;

	if (nheight > 2000) {
		nheight = 2000;
		nwidth = width * 2000 / height;
	}

	printf("scale to %d by %d\n", nwidth, nheight);
	unsigned char *outbuf;

	outbuf = malloc(2000 * 2000 * depth);

	int x, y, layer;
	for (y = 0; y < nheight; y++) {
		for (x = 0; x < nwidth; x++) {
			int origx = x * width / nwidth;
			int origy = y * height / nheight;

			int origoff = origx * depth +
			     	  origy * width * depth;

			int off = x * depth + y * nwidth * depth;

			for (layer = 0; layer < depth; layer++) {
				outbuf[off + layer] = image[origoff + layer];
			}
		}
	}

	output(outbuf, nwidth, nheight, depth, filename, 0, 0, nwidth, nheight);

	free(outbuf);

	return;
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
		 0, 0, cinfo.output_width, cinfo.output_height);
  free(image);

  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

}

int main(int argc, char **argv) {
	int i;

	for (i = 1; i < argc; i++) {
		read_JPEG_file(argv[i]);
	}
}
