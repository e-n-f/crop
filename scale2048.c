#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <png.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int wantwidth = 2048;
int wantheight = 2048;

void output(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom) {
	char fname[2000];
	int quality = 75;
	char *cp;
	char *basename = NULL;

	strcpy(fname, filename);
	for (cp = fname; *cp; cp++) {
		if (*cp == '/') {
			basename = cp;
		}
	}

	if (basename != NULL) {
		*basename = '\0';
	}
	strcat(fname, "/rescale/");

	mkdir(fname, 0755);

	basename = filename;
	for (cp = filename; *cp; cp++) {
		if (*cp == '/') {
			basename = cp + 1;
		}
	}

	strcat(fname, basename);

	printf("write %s\n", fname);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *outfile;
	JSAMPROW row_pointer[1];

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

	int y = top;
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &image[y * depth * width + left * depth];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
		y++;
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
}

void process(unsigned char *image, int width, int height, int depth, char *filename,
	     int left, int top, int right, int bottom) {
	printf("processing %d by %d, depth %d\n", width, height, depth);

	int nwidth, nheight;

	nwidth = wantwidth;
	nheight = height * wantwidth / width;

	if (nheight > wantheight) {
		nheight = wantheight;
		nwidth = width * wantheight / height;
	}

	printf("scale to %d by %d\n", nwidth, nheight);
	unsigned char *outbuf = malloc(wantwidth * wantheight * depth);

	int x, y, layer;
	for (y = 0; y < nheight; y++) {
		for (x = 0; x < nwidth; x++) {
			for (layer = 0; layer < depth; layer++) {
				if (nwidth >= width) {
					double origx = 1.0 * x * width / nwidth;
					double origy = 1.0 * y * height / nheight;

					int ox = origx;
					int oy = origy;

					double val1 = (ox + 1 - origx) * image[(ox     + oy * width) * depth + layer] +
						      (origx - ox    ) * image[(ox + 1 + oy * width) * depth + layer];
					double val2 = (ox + 1 - origx) * image[(ox     + (oy + 1) * width) * depth + layer] +
						      (origx - ox    ) * image[(ox + 1 + (oy + 1) * width) * depth + layer];
					double val =  (oy + 1 - origy) * val1 +
						      (origy - oy    ) * val2;

					int off = x * depth + y * nwidth * depth;
					outbuf[off + layer] = val;
				} else {
					double origx1 = 1.0 * x * width / nwidth;
					double origy1 = 1.0 * y * width / nwidth;
					int ox1 = origx1;
					int oy1 = origy1;

					double origx2 = 1.0 * (x + 1) * width / nwidth;
					double origy2 = 1.0 * (y + 1) * width / nwidth;
					int ox2 = origx2;
					int oy2 = origy2;

					double sum = 0;

					sum += image[(ox1 + oy1 * width) * depth + layer] * (ox1 + 1 - origx1) * (oy1 + 1 - origy1);
					sum += image[(ox2 + oy1 * width) * depth + layer] * (origx2 - ox2) * (oy1 + 1 - origy1);
					sum += image[(ox1 + oy2 * width) * depth + layer] * (ox1 + 1 - origx1) * (origy2 - oy2);
					sum += image[(ox2 + oy2 * width) * depth + layer] * (origx2 - ox2) * (origy2 - oy2);

					int xx, yy;

					for (xx = ox1 + 1; xx < ox2; xx++) {
						sum += image[(xx + oy1 * width) * depth + layer] * (oy1 + 1 - origy1);
						sum += image[(xx + oy2 * width) * depth + layer] * (origy2 - oy2);
					}

					for (yy = oy1 + 1; yy < oy2; yy++) {
						sum += image[(ox1 + yy * width) * depth + layer] * (ox1 + 1 - origx1);
						sum += image[(ox2 + yy * width) * depth + layer] * (origx2 - ox2);
					}

					for (xx = ox1 + 1; xx < ox2; xx++) {
						for (yy = oy1 + 1; yy < oy2; yy++) {
							sum += image[(xx + yy * width) * depth + layer];
						}
					}

					sum /= (origx2 - origx1) * (origy2 - origy1);

					int off = x * depth + y * nwidth * depth;
					outbuf[off + layer] = sum;
				}
			}
		}
	}

	output(outbuf, nwidth, nheight, depth, filename, 0, 0, nwidth, nheight);

	free(outbuf);
	return;
}

void read_png(char *filename)
{
	png_image image;

	memset(&image, 0, sizeof(image));
	image.version = PNG_IMAGE_VERSION;

	if (png_image_begin_read_from_file(&image, filename)) {
		png_bytep buffer;

		image.format = PNG_FORMAT_RGB;
		buffer = malloc(PNG_IMAGE_SIZE(image));

		if (buffer != NULL &&
		    png_image_finish_read(&image, NULL /* background */, buffer,
			0 /* row stride */, NULL /* colormap */)) {

			int n = strlen(filename) - 4;
			if (n >= 0) {
				if (strcmp(filename + n, ".png") == 0) {
					strcpy(filename + n, ".jpg");
				}
			}

			process(buffer, image.width, image.height, 3, filename,
				 0, 0, image.width, image.height);
			free(buffer);
		} else {
			if (buffer == NULL) {
				png_image_free(&image);
			} else {
				fprintf(stderr, "pngtopng: error: %s\n", image.message);
				free(buffer);
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
		 0, 0, cinfo.output_width, cinfo.output_height);
  free(image);

  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

}

int main(int argc, char **argv) {
	int i;
	extern int optind;
	extern char *optarg;

	while ((i = getopt(argc, argv, "s:")) != -1) {
		switch (i) {
		case 's':
			wantwidth = atoi(optarg);
			break;

		default:
			fprintf(stderr, "Usage: %s [-s size] files...\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	for (i = optind; i < argc; i++) {
		if (strstr(argv[i], ".jpg") || strstr(argv[i], ".jpeg") || strstr(argv[i], ".JPG")) {
			read_JPEG_file(argv[i]);
		} else {
			read_png(argv[i]);
		}
	}
}
