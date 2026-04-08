#include "compressor/serf_qt_compressor.h"

SerfQtCompressor::SerfQtCompressor(int block_size, double max_diff) :kBlockSize(block_size), kMaxDiff(max_diff * 0.999) {
  output_bit_stream_ = std::make_unique<OutputBitStream>(2 * block_size * 8);
}

void SerfQtCompressor::AddValue(double v) {
  if (first_) {
    first_ = false;
    compressed_size_in_bits_ += output_bit_stream_->WriteInt(kBlockSize, 16);
    compressed_size_in_bits_ += output_bit_stream_->WriteLong(Double::DoubleToLongBits(kMaxDiff), 64);
  }

  /*
  long q = static_cast<long>(std::round((v - pre_value_) / (2 * kMaxDiff)));
  double recoverValue = pre_value_ + 2 * kMaxDiff * static_cast<double>(q);
  compressed_size_in_bits_ += EliasGammaCodec::Encode(ZigZagCodec::Encode(static_cast<int64_t>(q)) + 1,
                                                      output_bit_stream_.get());
  pre_value_ = recoverValue;
  */

  // 两个调整值，可选择更接近的或者更符合惯性的那个
  long d_up = static_cast<long>(std::ceil((v - pre_value_) / kMaxDiff));
  long d_down = static_cast<long>(std::floor((v - pre_value_) / kMaxDiff));

  double app_up = pre_value_ + kMaxDiff * static_cast<double>(d_up);
  double app_down = pre_value_ + kMaxDiff * static_cast<double>(d_down);

  // 选择更接近的那个
  long q;
  if(std::abs(app_up - v) < std::abs(app_down - v)) {
    q = d_up;
    pre_value_ = app_up;
  } else {
    q = d_down;
    pre_value_ = app_down;
  }

  compressed_size_in_bits_ += EliasGammaCodec::Encode(ZigZagCodec::Encode(static_cast<int64_t>(q)) + 1,
                                                      output_bit_stream_.get());
  pre_value_ = recoverValue;
}

Array<uint8_t> SerfQtCompressor::compressed_bytes() {
  return compressed_bytes_;
}

void SerfQtCompressor::Close() {
  output_bit_stream_->Flush();
  compressed_bytes_ = output_bit_stream_->GetBuffer(std::ceil(compressed_size_in_bits_ / 8.0));
  output_bit_stream_->Refresh();
  first_ = true;
  pre_value_ = 2;
  stored_compressed_size_in_bits_ = compressed_size_in_bits_;
  compressed_size_in_bits_ = 0;
}

long SerfQtCompressor::get_compressed_size_in_bits() const {
  return stored_compressed_size_in_bits_;
}
