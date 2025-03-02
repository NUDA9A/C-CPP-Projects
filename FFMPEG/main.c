#include "return_codes.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>

#include <fftw3.h>
#include <stdio.h>
#define DBL_MIN_VAL 2.22507e-308

typedef struct MyAVFormatContext
{
	int nb_channels;
	int sample_rate;
	const AVCodec* codec;
	AVFormatContext* fmtContext;
	int stream_index;
	AVCodecContext* codecContext;
} MyAVFormatContext;

void print_error(int* return_code, int code)
{
	switch (code)
	{
	case ERROR_CANNOT_OPEN_FILE:
		fprintf(stderr, "Can not open file.");
		*return_code = ERROR_CANNOT_OPEN_FILE;
		break;
	case ERROR_NOTENOUGH_MEMORY:
		fprintf(stderr, "Not enough memory.");
		*return_code = ERROR_NOTENOUGH_MEMORY;
		break;
	case ERROR_DATA_INVALID:
		fprintf(stderr, "Data is invalid.");
		*return_code = ERROR_DATA_INVALID;
		break;
	case ERROR_ARGUMENTS_INVALID:
		fprintf(stderr, "Arguments are invalid.");
		*return_code = ERROR_ARGUMENTS_INVALID;
		break;
	case ERROR_FORMAT_INVALID:
		fprintf(stderr, "Format is invalid.");
		*return_code = ERROR_FORMAT_INVALID;
		break;
	case ERROR_UNKNOWN:
		fprintf(stderr, "Unknown error.");
		*return_code = ERROR_UNKNOWN;
		break;
	default:
		break;
	}
}

int read_file(MyAVFormatContext* formatContext, char* filename)
{
	int return_code = 0;
	AVFormatContext* file = avformat_alloc_context();
	if (!file)
	{
		print_error(&return_code, ERROR_NOTENOUGH_MEMORY);
		goto end;
	}
	if (avformat_open_input(&file, filename, NULL, NULL) < 0 || avformat_find_stream_info(file, NULL) < 0)
	{
		print_error(&return_code, ERROR_CANNOT_OPEN_FILE);
		goto end;
	}

	const char* format = file->iformat->name;

	if (!(!strcmp(format, "mpeg") || !strcmp(format, "mp3") || !strcmp(format, "flac") || !strcmp(format, "ogg") || !strcmp(format, "aac")))
	{
		print_error(&return_code, ERROR_FORMAT_INVALID);
		goto end;
	}

	(*formatContext).stream_index = -1;
	for (int i = 0; i < file->nb_streams; i++)
	{
		AVCodecParameters* curr = file->streams[i]->codecpar;
		if (curr->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			(*formatContext).sample_rate = curr->sample_rate;
			(*formatContext).nb_channels = curr->ch_layout.nb_channels;
			(*formatContext).codec = avcodec_find_decoder(curr->codec_id);

			if (formatContext->codec == NULL)
			{
				print_error(&return_code, ERROR_DATA_INVALID);
				goto end;
			}
			AVCodecContext* codecContext = avcodec_alloc_context3(formatContext->codec);
			if (codecContext == NULL)
			{
				print_error(&return_code, ERROR_NOTENOUGH_MEMORY);
				goto end;
			}
			if (avcodec_parameters_to_context(codecContext, curr) < 0)
			{
				avcodec_free_context(&codecContext);
				print_error(&return_code, ERROR_UNKNOWN);
				goto end;
			}
			if (avcodec_open2(codecContext, formatContext->codec, NULL) < 0)
			{
				avcodec_free_context(&codecContext);
				print_error(&return_code, ERROR_UNKNOWN);
				goto end;
			}

			(*formatContext).codecContext = codecContext;
			(*formatContext).fmtContext = file;
			(*formatContext).stream_index = i;
			break;
		}
	}

	if (formatContext->stream_index == -1)
	{
		print_error(&return_code, ERROR_DATA_INVALID);
		goto end;
	}

end:
	if (!return_code)
	{
		return_code = SUCCESS;
	}

	return return_code;
}

int my_realloc(double** buf, int* size)
{
	if (buf)
	{
		double* new_buf = realloc((*buf), (*size) * sizeof(double));
		if (new_buf == NULL)
		{
			free(*buf);
			fprintf(stderr, "Can not reallocate memory.");
			return ERROR_NOTENOUGH_MEMORY;
		}
		(*buf) = new_buf;
	}

	return SUCCESS;
}

int handleFrame(SwrContext* swrContext, AVCodecContext* codecCtx, AVFrame* frame, double** buf1, double** buf2, int* size, int* res_size)
{
	uint8_t* data[8];
	data[0] = (double*)malloc(frame->nb_samples * sizeof(double));
	data[1] = (double*)malloc(frame->nb_samples * sizeof(double));

	int max_samples = swr_get_out_samples(swrContext, frame->nb_samples);
	if (max_samples <= 0)
	{
		return ERROR_UNKNOWN;
	}

	int ret = swr_convert(swrContext, (uint8_t**)data, max_samples, (const uint8_t**)frame->data, frame->nb_samples);
	if (ret <= 0)
	{
		fprintf(stderr, "Can not convert frame.");
		return ERROR_UNKNOWN;
	}

	double* ch1_buf = (double*)data[0];
	double* ch2_buf = (double*)data[1];

	if (ch1_buf == NULL || ch2_buf == NULL)
	{
		fprintf(stderr, "Can not allocate memory.");
		return ERROR_NOTENOUGH_MEMORY;
	}

	for (int i = 0; i < ret; i++)
	{
		if (i + *res_size == *size)
		{
			*size *= 2;
			if (my_realloc(buf1, size) || my_realloc(buf2, size))
			{
				fprintf(stderr, "Can not reallocate memory.");
				return ERROR_NOTENOUGH_MEMORY;
			}
		}
		(*buf1)[i + *res_size] = ch1_buf[i];
		if (buf2)
		{
			(*buf2)[i + *res_size] = ch2_buf[i];
		}
	}

	*res_size += ret;
	return SUCCESS;
}

int receiveAndHandle(AVCodecContext* codecCtx, AVFrame* frame, SwrContext* swrContext, double** buf1, double** buf2, int* size, int* res_size)
{
	int ret = SUCCESS;

	while ((ret = avcodec_receive_frame(codecCtx, frame)) == 0)
	{
		int return_code = handleFrame(swrContext, codecCtx, frame, buf1, buf2, size, res_size);
		if (return_code)
		{
			return return_code;
		}

		av_frame_unref(frame);
	}
	return ret;
}

int drainDecoder(AVCodecContext* codecCtx, AVFrame* frame, SwrContext* swrContext, double** buf1, double** buf2, int* size, int* res_size)
{
	int ret = 0;
	if ((ret = avcodec_send_packet(codecCtx, NULL)) == 0)
	{
		ret = receiveAndHandle(codecCtx, frame, swrContext, buf1, buf2, &size, &res_size);
		if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		{
			fprintf(stderr, "Error while receiving a frame from the decoder\n");
			return ERROR_UNKNOWN;
		}
	}
	else
	{
		fprintf(stderr, "Error sending a packet for decoding\n");
		return ERROR_UNKNOWN;
	}
	return SUCCESS;
}

int read_data(MyAVFormatContext* formatContext, double** buf_1, double** buf_2, int* size, int* res_size, int max_sample_rate)
{
	int new_sample_rate = max_sample_rate != 0 ? max_sample_rate : formatContext->sample_rate;

	AVPacket* packet = av_packet_alloc();

	AVFrame* frame = av_frame_alloc();
	if (!frame || !packet)
	{
		fprintf(stderr, "Can not allocate memory for frame.");
		return ERROR_NOTENOUGH_MEMORY;
	}

	SwrContext* swrContext = swr_alloc();

	if (!swrContext)
	{
		fprintf(stderr, "Can not allocate memory for swrContext.");
		return ERROR_NOTENOUGH_MEMORY;
	}

	if (swr_alloc_set_opts2(
			&swrContext,
			&formatContext->codecContext->ch_layout,
			AV_SAMPLE_FMT_DBLP,
			new_sample_rate,
			&formatContext->codecContext->ch_layout,
			formatContext->codecContext->sample_fmt,
			formatContext->sample_rate,
			0,
			NULL) < 0)
	{
		av_packet_free(&packet);
		av_frame_free(&frame);
		fprintf(stderr, "Can not initialize swrContext.");
		return ERROR_UNKNOWN;
	}

	if (swr_init(swrContext) < 0)
	{
		fprintf(stderr, "Can not initialize swrContext.");
		return ERROR_UNKNOWN;
	}

	int ret = 0;
	while ((ret = av_read_frame(formatContext->fmtContext, packet)) != AVERROR_EOF)
	{
		if (ret != 0)
		{
			fprintf(stderr, "Can not read frame.");
			break;
		}

		if (packet->stream_index != formatContext->stream_index)
		{
			av_packet_unref(packet);
			continue;
		}

		if ((ret = avcodec_send_packet(formatContext->codecContext, packet)) == 0)
		{
			av_packet_unref(packet);
		}
		else
		{
			fprintf(stderr, "Can not send packet to decoder.");
			break;
		}

		if ((ret = receiveAndHandle(formatContext->codecContext, frame, swrContext, buf_1, buf_2, size, res_size)) != AVERROR(EAGAIN))
		{
			fprintf(stderr, "Something went wrong.");
			break;
		}
	}

	int return_code = drainDecoder(formatContext->codecContext, frame, swrContext, buf_1, buf_2, size, res_size);
	if (return_code)
	{
		return return_code;
	}

	av_packet_free(&packet);
	av_frame_free(&frame);
	swr_free(&swrContext);

	avcodec_free_context(&formatContext->codecContext);
	avformat_close_input(&formatContext->fmtContext);
	if (ret != 0 && ret != AVERROR_EOF)
	{
		return ERROR_UNKNOWN;
	}

	return SUCCESS;
}

int cross_correlation(double** buf1, double** buf2, int size1, int size2, int* time_samples_shift)
{
	int max_size = size1 >= size2 ? size1 : size2;

	if (my_realloc(buf1, &max_size) || my_realloc(buf2, &max_size))
	{
		return ERROR_NOTENOUGH_MEMORY;
	}

	fftw_complex* x_res = fftw_malloc((max_size / 2 + 1) * sizeof(fftw_complex));
	fftw_complex* y_res = fftw_malloc((max_size / 2 + 1) * sizeof(fftw_complex));
	fftw_complex* pre_res = fftw_malloc((max_size / 2 + 1) * sizeof(fftw_complex));

	double* x = (double*)fftw_alloc_real(max_size * sizeof(double));
	double* y = (double*)fftw_alloc_real(max_size * sizeof(double));
	double* res = (double*)fftw_alloc_real(sizeof(double) * max_size);

	if (!x_res || !y_res || !x || !y)
	{
		fprintf(stderr, "Can not allocate memory.");
		return ERROR_NOTENOUGH_MEMORY;
	}

	for (int i = 0; i < max_size; i++)
	{
		x[i] = (*buf1)[i];
		y[i] = (*buf2)[i];
	}

	free((*buf1));
	free((*buf2));

	fftw_plan plan_x = fftw_plan_dft_r2c_1d(max_size, x, x_res, FFTW_ESTIMATE);
	fftw_plan plan_y = fftw_plan_dft_r2c_1d(max_size, y, y_res, FFTW_ESTIMATE);

	fftw_execute(plan_x);
	fftw_execute(plan_y);

	fftw_free(x);
	fftw_free(y);

	for (int k = 0; k < max_size / 2 + 1; k++)
	{
		pre_res[k][0] = x_res[k][0] * y_res[k][0] + x_res[k][1] * y_res[k][1];
		pre_res[k][1] = -x_res[k][0] * y_res[k][1] + x_res[k][1] * y_res[k][0];
	}

	fftw_free(x_res);
	fftw_free(y_res);

	fftw_plan plan_back_x = fftw_plan_dft_c2r_1d(max_size, pre_res, res, FFTW_ESTIMATE);
	fftw_execute(plan_back_x);

	double max_value = DBL_MIN_VAL;
	int max_index = 0;

	for (int k = 0; k < max_size; k++)
	{
		if (res[k] > max_value)
		{
			max_value = res[k];
			max_index = k;
		}
	}

	*time_samples_shift = max_index;

	if (max_index > size1)
	{
		*time_samples_shift = max_index - max_size;
	}

	fftw_destroy_plan(plan_x);
	fftw_destroy_plan(plan_y);
	fftw_destroy_plan(plan_back_x);

	fftw_free(pre_res);
	fftw_free(res);
	fftw_cleanup();

	return SUCCESS;
}

int main(int argc, char* argv[])
{
	av_log_set_level(AV_LOG_QUIET);
	if (argc == 2)
	{
		int time_samples_shift;
		MyAVFormatContext file1;
		int return_code = read_file(&file1, argv[1]);
		if (return_code)
		{
			return return_code;
		}

		if (file1.nb_channels != 2)
		{
			fprintf(stderr, "Invalid data.");
			return ERROR_FORMAT_INVALID;
		}

		int res_size = 0, bufSize = 10;
		double* channel1_buf = (double*)malloc(bufSize * sizeof(double));
		double* channel2_buf = (double*)malloc(bufSize * sizeof(double));
		if (!channel1_buf || !channel2_buf)
		{
			fprintf(stderr, "Can not allocate memory.");
			return ERROR_NOTENOUGH_MEMORY;
		}

		return_code = read_data(&file1, &channel1_buf, &channel2_buf, &bufSize, &res_size, 0);
		if (return_code)
		{
			fprintf(stderr, "Something went wrong.");
			return return_code;
		}

		return_code = cross_correlation(&channel1_buf, &channel2_buf, res_size, res_size, &time_samples_shift);
		if (return_code)
		{
			free(channel1_buf);
			free(channel2_buf);
			return return_code;
		}

		printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n",
			   time_samples_shift,
			   file1.sample_rate,
			   time_samples_shift * 1000 / file1.sample_rate);
	}
	else if (argc == 3)
	{
		int time_samples_shift;
		MyAVFormatContext file1, file2;
		int return_code1 = read_file(&file1, argv[1]);
		int return_code2 = read_file(&file2, argv[2]);

		if (return_code1 || return_code2)
		{
			return return_code1 ? return_code1 : return_code2;
		}

		int max_sample_rate = 0;
		if (file1.sample_rate != file2.sample_rate)
		{
			max_sample_rate = file1.sample_rate > file2.sample_rate ? file1.sample_rate : file2.sample_rate;
		}

		int res1_size = 0, res2_size = 0, buf1_size = 10, buf2_size = 10;
		double* buf1 = (double*)malloc(buf1_size * sizeof(double));
		double* buf2 = (double*)malloc(buf2_size * sizeof(double));
		if (!buf1 || !buf2)
		{
			fprintf(stderr, "Can not allocate memory.");
			return ERROR_NOTENOUGH_MEMORY;
		}

		return_code1 = read_data(&file1, &buf1, NULL, &buf1_size, &res1_size, max_sample_rate);
		return_code2 = read_data(&file2, &buf2, NULL, &buf2_size, &res2_size, max_sample_rate);

		if (return_code1 || return_code2)
		{
			return return_code1 ? return_code1 : return_code2;
		}

		return_code1 = cross_correlation(&buf1, &buf2, res1_size, res2_size, &time_samples_shift);
		if (return_code1)
		{
			free(buf1);
			free(buf2);
			return return_code1;
		}
		max_sample_rate = max_sample_rate ? max_sample_rate : file1.sample_rate;
		printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", time_samples_shift, max_sample_rate, time_samples_shift * 1000 / max_sample_rate);
	}
	else
	{
		fprintf(stderr, "Invalid arguments amount.");
		return ERROR_ARGUMENTS_INVALID;
	}

	return SUCCESS;
}
