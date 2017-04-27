#include <cassert>
#include <sstream>
#include "matrix.h"
#include "matrix_functions.h"
#include "types-fpga.h"
#include "kernel.h"
#include "common/exception.h"

using namespace std;

namespace amunmt {
namespace FPGA {
namespace mblas {

Matrix::Matrix(const OpenCLInfo &openCLInfo)
:dims_({0, 0, 0, 0})
,arr_(openCLInfo)
{
}

Matrix::Matrix(const OpenCLInfo &openCLInfo, size_t rows, size_t cols, bool zero)
:dims_({rows, cols, 1, 1})
,arr_(openCLInfo, size())
{
  if (zero) {
    arr_.Set(0);
  }

}

Matrix::Matrix(const OpenCLInfo &openCLInfo, size_t rows, size_t cols, float *val)
:dims_({rows, cols, 1, 1})
,arr_(openCLInfo, size(), val)
{
}

Matrix::Matrix(const Matrix &other)
:dims_({other.dims_[0], other.dims_[1], other.dims_[2], other.dims_[3]})
,arr_(other.arr_)
{
}

Matrix::Matrix(Matrix &&other)
:Matrix(other.GetOpenCLInfo())
{
  Swap(other);
}


Matrix::~Matrix()
{
}

void Matrix::Resize(size_t rows, size_t cols, size_t beam, size_t batches)
{
  size_t newSize = cols * rows * beam * batches;
  arr_.resize(newSize);

  dims_[0] = rows;
  dims_[1] = cols;
  dims_[2] = beam;
  dims_[3] = batches;
}

void Matrix::Reshape(size_t rows, size_t cols, size_t beam, size_t batches)
{
  size_t newSize = cols * rows * beam * batches;
  amunmt_UTIL_THROW_IF2(newSize > arr_.size(), "Must reshape to same or smaller size");

  dims_[0] = rows;
  dims_[1] = cols;
  dims_[2] = beam;
  dims_[3] = batches;
}

void Matrix::Reshape2D()
{
  dims_[0] = dims_[0] * dims_[2] * dims_[3];
  dims_[2] = 1;
  dims_[3] = 1;
}


std::string Matrix::Debug(size_t verbosity) const
{
  std::stringstream strm;
  strm << BaseMatrix::Debug(verbosity) << " " << arr_.Debug(verbosity);
  //cerr << "Debug1=" << strm.str() << endl;

  if (verbosity) {
    //cerr << "Debug2" << endl;
    float sum = SumFloat(arr_.GetOpenCLInfo(), arr_.data(), size());
    //cerr << "Debug3" << endl;
    strm << " sum=" << sum << std::flush;
    //cerr << "Debug4" << endl;

    if (verbosity == 2) {
      strm << " " << OutputArray<float>(arr_.GetOpenCLInfo(), arr_.data(), size());
    }
  }
  //cerr << "Debug5" << endl;

  return strm.str();
}

void Matrix::Swap(Matrix &other)
{
  assert(&arr_.GetOpenCLInfo() == &other.arr_.GetOpenCLInfo());
  std::swap(dims_, other.dims_);
  arr_.Swap(other.arr_);
}

void Matrix::Set(const float *data)
{
  //cerr << "Set1=" << size() << endl;
  CheckError( clEnqueueWriteBuffer(arr_.GetOpenCLInfo().commands, arr_.data(), CL_TRUE, 0, sizeof(float) * size(), data, 0, NULL, NULL) );
  //cerr << "Set2=" << size() << endl;
}

}
}
}
