#include "fast_buffer.h"

BEGIN_XTP_NAMESPACE(Szseo)

void FastBuffer::DestroyAll() {
	if (pos_begin_) {
		free(pos_begin_);
		pos_end_ = pos_writing_ = pos_reading_ = pos_begin_ = NULL;
	}
}

size_t FastBuffer::FindBytes(const void *pattern, size_t len, int from) {
	if (from < 0 || len <= 0) return -1;

	const char *p = (const char *)pattern;
	size_t dLen = pos_writing_ - pos_reading_ - len;

	for (size_t i = from; i <= dLen; ++i) {
		if (pos_reading_[i] == p[0] && \
			memcmp(pos_reading_ + i, p, len) == 0) return i;
	}
	return -1;
}

bool FastBuffer::ShrinkSpace(size_t max_size) {
	if (pos_begin_ == NULL) return true;

	// is the whole space too small?
	if (pos_end_ < pos_begin_ + max_size) return true;

	// is the data space too big?
	if (pos_writing_ > pos_reading_ + max_size) return true;

	size_t dlen = pos_writing_ - pos_reading_;
	if (dlen < 0) dlen = 0; // impossible!

	char *newbuf = (char *)malloc(max_size);
	if (!newbuf) return false;

	if (dlen > 0) {
		memcpy(newbuf, pos_reading_, dlen);
	}

	free(pos_begin_);
	pos_reading_ = pos_begin_ = newbuf;
	pos_writing_ = pos_begin_ + dlen;
	pos_end_ = pos_begin_ + max_size;
	return true;
}

bool FastBuffer::EnsureSize(size_t need) {
	if (pos_begin_ == NULL) {
		size_t len = 256;
		while (len < need) len <<= 1;

		pos_begin_ = (char *)malloc(len);
		if (!pos_begin_) return false;

		pos_writing_ = pos_reading_ = pos_begin_;
		pos_end_ = pos_begin_ + len;
		return true;
	}

	// is the writing size big enough?
	if (pos_end_ >= pos_writing_ + need) return true;

	size_t flen = (pos_end_ - pos_writing_) + \
		(pos_reading_ - pos_begin_);
	size_t dlen = pos_writing_ - pos_reading_;

	// not enough, or the idle is below 20%:
	if (flen < need || flen * 4 < dlen) {
		size_t bufsize = (pos_end_ - pos_begin_) * 2;
		while (bufsize - dlen < need) bufsize <<= 1;

		char *newbuf = (char *)malloc(bufsize);
		if (!newbuf) return false;

		if (dlen > 0) {
			memcpy(newbuf, pos_reading_, dlen);
		}
		free(pos_begin_);

		pos_reading_ = pos_begin_ = newbuf;
		pos_writing_ = pos_begin_ + dlen;
		pos_end_ = pos_begin_ + bufsize;
	}
	else {
		memmove(pos_begin_, pos_reading_, dlen);
		pos_reading_ = pos_begin_;
		pos_writing_ = pos_begin_ + dlen;
	}
	return true;
}


END_XTP_NAMESPACE(Szseo)