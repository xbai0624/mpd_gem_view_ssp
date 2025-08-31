#include "matrix.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <cmath>

namespace tracking_dev
{
    Matrix::Matrix()
    {
        Clear();
    }

    Matrix::Matrix(uint32_t i, uint32_t j)
    {
        dimension.first = i;
        dimension.second = j;
        _M.clear();
    }

    Matrix::Matrix(uint32_t i, uint32_t j, double v)
    {
        dimension.first = i;
        dimension.second = j;
        _M.clear();

        for (uint32_t r = 0; r < i; r++)
            (*this)(r, r) = v;
    }

    Matrix::Matrix(const std::vector<double> &v)
    {
        uint32_t i = v.size();
        dimension.first = i;
        dimension.second = 1;

        for (uint32_t r = 0; r < i; r++)
            (*this)(r, 0) = v.at(r);
    }

    // copy assignment
    Matrix &Matrix::operator=(const Matrix &m)
    {
        if (this == &m)
            return *this;

        const auto &d = m.GetDimension();
        dimension = d;

        _M.clear();

        const auto &t = m.GetCache();
        for (auto &i : t)
        {
            _M[i.first] = i.second;
        }

        return *this;
    }

    // copy structor
    Matrix::Matrix(const Matrix &m)
    {
        *this = m;
    }

    // move assignment
    Matrix &Matrix::operator=(Matrix &&m)
    {
        const auto &d = m.GetDimension();
        dimension = std::move(d);

        const auto &c = m.GetCache();
        _M = std::move(c);

        return *this;
    }

    // move constructor
    Matrix::Matrix(Matrix &&m)
    {
        *this = std::move(m);
    }

    Matrix::~Matrix()
    {
        Clear();
    }

    void Matrix::Clear()
    {
        dimension.first = 0;
        dimension.second = 0;
        _M.clear();
    }

    void Matrix::Zero()
    {
        _M.clear();
    }

    bool Matrix::DimensionEqual(const Matrix &m)
    {
        const auto &d = m.GetDimension();

        if (d.first == dimension.first && d.second == dimension.second)
            return true;
        return false;
    }

    Matrix Matrix::operator+(const Matrix &m)
    {
        if (!DimensionEqual(m))
        {
            std::cout << "Error: Adding two matrices with different dimension." << std::endl;
            exit(0);
        }

        Matrix res(dimension.first, dimension.second);

        for (uint32_t i = 0; i < dimension.first; i++)
            for (uint32_t j = 0; j < dimension.second; j++)
            {
                double r = (*this).at(i, j) + m.at(i, j);
                if (r != 0)
                    res(i, j) = r;
            }

        return res;
    }

    Matrix Matrix::operator-(const Matrix &m)
    {
        if (!DimensionEqual(m))
        {
            std::cout << "Error: Subtracting two matrices with different dimension." << std::endl;
            exit(0);
        }

        Matrix res(dimension.first, dimension.second);

        for (uint32_t i = 0; i < dimension.first; i++)
            for (uint32_t j = 0; j < dimension.second; j++)
            {
                double r = (*this).at(i, j) - m.at(i, j);
                if (r != 0)
                    res(i, j) = r;
            }

        return res;
    }

    Matrix Matrix::operator*(const Matrix &m)
    {
        const auto &d = m.GetDimension();

        if (dimension.second != d.first)
        {
            std::cout << "Error: Multiplying two matrices with unmatched dimension." << std::endl;
            exit(0);
        }

        Matrix res(dimension.first, d.second);

        for (uint32_t i = 0; i < dimension.first; i++)
        {
            for (uint32_t j = 0; j < d.second; j++)
            {
                double r = 0;
                for (uint32_t k = 0; k < dimension.second; k++)
                {
                    r += (*this).at(i, k) * m.at(k, j);
                }
                if (r != 0)
                    res(i, j) = r;
            }
        }

        return res;
    }

    Matrix Matrix::operator*(const double &r)
    {
        Matrix res(dimension.first, dimension.second);

        for (uint32_t i = 0; i < dimension.first; i++)
        {
            for (uint32_t j = 0; j < dimension.second; j++)
            {
                if ((*this).at(i, j) != 0)
                    res(i, j) = (*this).at(i, j) * r;
            }
        }

        return res;
    }

    Matrix Matrix::operator/(const Matrix &m)
    {
        Matrix inv = m.Inverse();
        Matrix res = inv * (*this);
        return res;
    }

    // Gauss-Jordan algorithm to find matrix inverse
    Matrix Matrix::Inverse() const
    {
        if (dimension.first <= 0 || dimension.second <= 0)
        {
            std::cout << "0 - Error: inversing a 0-dim matrix." << std::endl;
            exit(0);
        }
        Matrix ret(dimension.first, dimension.second);

        // 1, create augmented matrix
        Matrix tmp_res(dimension.first, 2 * dimension.second);
        for (uint32_t i = 0; i < dimension.first; i++)
        {
            for (uint32_t j = 0; j < dimension.second; j++)
            {
                if ((*this).at(i, j) != 0)
                    tmp_res(i, j) = (*this).at(i, j);
            }

            tmp_res(i, dimension.second + i) = 1;
        }
        // std::cout<<"augmented matrix:"<<std::endl;
        // std::cout<<tmp_res<<std::endl;

        // 2, interchange row of matrix.
        //    Purpose: make the diagnal element the largest one (absolute value),
        //             technically larger than all rows below it
        for (uint32_t i = 0; i < dimension.first - 1; i++)
        {
            for (uint32_t ii = i + 1; ii < dimension.first; ii++)
            {
                if (abs(tmp_res.at(i, i)) >= abs(tmp_res(ii, i)))
                    continue;

                for (uint32_t j = 0; j < 2 * dimension.second; j++)
                {
                    double lower_row_temp = tmp_res.at(ii, j);
                    double upper_row_temp = tmp_res.at(i, j);

                    if (lower_row_temp == 0 && upper_row_temp == 0)
                        continue;

                    tmp_res(ii, j) = upper_row_temp;
                    tmp_res(i, j) = lower_row_temp;
                }
            }
        }
        // std::cout<<"row changed matrix:"<<std::endl;
        // std::cout<<tmp_res<<std::endl;        

        // 4, zero row elements except for the diagnal element, which should be 1
        for (uint32_t i1 = 0; i1 < dimension.first; i1++)
        {
            if (tmp_res.at(i1, i1) == 0)
            {
                std::cout << "1 - Warning: diagnal element (" << i1 << ", " << i1 << ") = 0; " << (*this).at(i1, i1) << std::endl;
                std::cout << "1 - Warning: matrix doesn't have inverse, return 0 instead." << std::endl;
                std::cout << tmp_res << std::endl;
                return ret;
            }

            for (uint32_t i2 = 0; i2 < dimension.second; i2++)
            {
                if (i1 == i2)
                    continue;

                double ratio = tmp_res.at(i2, i1) / tmp_res.at(i1, i1);
                for (uint32_t j = 0; j < dimension.second * 2; j++)
                    tmp_res(i2, j) = tmp_res.at(i2, j) - tmp_res.at(i1, j) * ratio;
            }
        }
        // std::cout<<"reselut matrix:"<<std::endl;
        // std::cout<<tmp_res<<std::endl;  

        // 3, divide row element by diagonal element
        for (uint32_t i = 0; i < dimension.first; i++)
        {
            double s = tmp_res.at(i, i);
            if (s == 0)
            {
                std::cout << "2 - Warning: matrix doesn't have an inverse, return 0 instead." << std::endl;
                return ret;
            }

            for (uint32_t j = 0; j < dimension.second * 2; j++)
                tmp_res(i, j) = tmp_res.at(i, j) / s;
        }
        // std::cout<<"row normalized matrix:"<<std::endl;
        // std::cout<<tmp_res<<std::endl;  

        // return result
        for (uint32_t i = 0; i < dimension.first; i++)
        {
            for (uint32_t j = dimension.second; j < dimension.second * 2; j++)
                if (tmp_res.at(i, j) != 0)
                    ret(i, j - dimension.second) = tmp_res.at(i, j);
        }

        return ret;
    }

    Matrix Matrix::Transpose() const
    {
        Matrix res(dimension.second, dimension.first);

        for (uint32_t i = 0; i < dimension.first; i++)
        {
            for (uint32_t j = 0; j < dimension.second; j++)
            {
                double v = (*this).at(i, j);
                if (v != 0)
                    res(j, i) = v;
            }
        }

        return res;
    }

    // get matrix section, [r1, r2), [c1, c2) :
    // inclusive for left side margin, exclusive for right side margin
    Matrix Matrix::GetSection(const uint32_t &r1, const uint32_t &r2,
                              const uint32_t &c1, const uint32_t &c2)
    {
        if (r2 < r1 || c2 < c1)
        {
            std::cout << "Error: matrix GetSection() invalid margin." << std::endl;
            exit(0);
        }

        Matrix res(r2 - r1, c2 - c1);

        for (unsigned int i = r1; i < r2; i++)
        {
            for (unsigned int j = c1; j < c2; j++)
            {
                if ((*this).at(i, j) != 0)
                    res(i - r1, j - c1) = (*this).at(i, j);
            }
        }

        return res;
    }

    void Matrix::PrintDimension() const
    {
        std::cout << "(" << dimension.first << ", " << dimension.second << ")" << std::endl;
    }

    std::ostream &operator<<(std::ostream &os, const Matrix &m)
    {
        const auto &d = m.GetDimension();

        for (uint32_t i = 0; i < d.first; i++)
        {
            for (uint32_t j = 0; j < d.second; j++)
            {
                os << std::setfill(' ') << std::setw(12) << std::setprecision(4) << m.at(i, j) << ",";
            }
            os << std::endl;
        }

        return os;
    }
};
