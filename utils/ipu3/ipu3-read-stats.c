/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ipu3-read-stats - Dump IPU3 statistics
 *
 * Copyright 2022 Umang Jain <umang.jain@ideasonboard.com>
 */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../include/linux/intel-ipu3.h"

static void usage(const char *argv0)
{
	printf("Usage: %s input-file output-file\n", basename(argv0));
	printf("Dump IPU3 statistics\n");
	printf("If the output-file '-', output data will be written to standard output\n");
}

int main(int argc, char *argv[])
{
	int in_fd;
	FILE *fout;
	int ret;

	if (argc != 3) {
		usage(argv[0]);
		return 1;
	}

	in_fd = open(argv[1], O_RDONLY);
	if (in_fd == -1) {
		fprintf(stderr, "Failed to open statistics file '%s': %s\n",
			argv[1], strerror(errno));
		return 1;
	}

	if (strcmp(argv[2], "-") == 0) {
		fout = stdout;
	} else {
		fout = fopen(argv[2], "w");
		if (!fout) {
			fprintf(stderr, "Failed to open output file '%s': %s\n",
				argv[2], strerror(errno));
			close(in_fd);
			return 1;
		}
	}

	int frame = 0;
	struct ipu3_uapi_stats_3a *stats = (struct ipu3_uapi_stats_3a *) malloc(sizeof(struct ipu3_uapi_stats_3a));

	while (1) {
		ret = read(in_fd, stats, sizeof(struct ipu3_uapi_stats_3a));
		if (ret < 0) {
			fprintf(stderr, "Failed to read stats data: %s\n",
				strerror(errno));
			goto done;
		}

		if ((unsigned)ret < sizeof(struct ipu3_uapi_stats_3a)) {
			if (ret != 0)
				fprintf(stderr, "%u bytes of stray data at end of input\n",
					ret);
			goto done;
		}

		/* ================ Dump AE stats ================ */


		/* ================ Dump AWB stats ================ */

		/* Sum the per-channel averages */
		unsigned int stride_ = 56; /* \todo find dynamic way to calculate grid stride */
		double redSum = 0, greenSum = 0, blueSum = 0;
		struct ipu3_uapi_grid_config grid = stats->stats_4a_config.awb_config.grid;
		for (unsigned int cellY = 0; cellY < grid.height; cellY++) {
			for (unsigned int cellX = 0; cellX < grid.width; cellX++) {
				uint32_t cellPosition = cellY * stride_ + cellX;

				const struct ipu3_uapi_awb_set_item *cell =
					(const struct ipu3_uapi_awb_set_item *)(
						&stats->awb_raw_buffer.meta_data[cellPosition]
					);
				const uint8_t G_avg = (cell->Gr_avg + cell->Gb_avg) / 2;

				redSum += cell->R_avg;
				greenSum += G_avg;
				blueSum += cell->B_avg;
			}
		}

		fprintf(fout, "Frame: %d, redSum: %.1f, greenSum: %.1f, blueSum: %.1f\n",
			frame, redSum, greenSum, blueSum);
		frame++;
	}

done:
	free(stats);
	close(in_fd);
	fclose(fout);

	return ret ? 1 : 0;
}
