#ifndef MATRIX_H
#define MATRIX_H

/*
 * designed with sparse matrix in mind
 *
 * Matrix will only use double precision
 *
 * - as of speed, double precision is identical to float precision
 *
 * - as of memory, double precision consumes twice large of memory
 *   space, but since we don't know how our matrix looks like, it might be
 *   a matrix with a large condition number, in that case, single
 *   precision will generate unreliable results, so using only double
 *   precision is worth the cost
 *
 * - map key only use uint32_t
 *
 */

#include <unordered_map>
#include <vector>
#include <ostream>

namespace tracking_dev
{
    class Matrix
    {
    public:
        struct pair_hasher
        {
            uint64_t operator()(const std::pair<uint32_t, uint32_t> &k) const
            {
                return (
                        (static_cast<uint64_t>(k.first) << 32)
                        ^ static_cast<uint64_t>(k.second)
                        );
            }
        };

    public:
        Matrix();
        Matrix(uint32_t, uint32_t);
        Matrix(uint32_t, uint32_t, double val);
        Matrix(const std::vector<double> &v);
        Matrix(const Matrix &m);
        Matrix &operator=(const Matrix &m);
        Matrix(Matrix &&m);
        Matrix &operator=(Matrix &&m);

        ~Matrix();
        void Clear(); // clear set dimenion to 0
        void Zero();  // zero keeps dimension, but set every element to 0
        bool DimensionEqual(const Matrix &m);
        void PrintDimension() const;

        const std::pair<uint32_t, uint32_t> & GetDimension() const
        {
            return dimension;
        }
        void SetDimension(const std::pair<uint32_t, uint32_t> &d)
        {
            dimension = d;
        }
        void SetDimension(const uint32_t &r, const uint32_t &c)
        {
            dimension.first = r; dimension.second = c;
        }

        double at(const uint32_t &i, const uint32_t &j) const
        {
            std::pair<uint32_t, uint32_t> key(i, j);

            if(_M.find(key) == _M.end())
                return 0;
            return _M.at(key);
        }

        double &operator()(const uint32_t &i, const uint32_t &j)
        {
            std::pair<uint32_t, uint32_t> key(i, j);

            return _M[key];
        }

        const std::unordered_map<std::pair<uint32_t, uint32_t>, double, pair_hasher> &
            GetCache() const {
                return _M;
            }

        Matrix operator+(const Matrix &m);
        Matrix operator-(const Matrix &m);
        Matrix operator*(const Matrix &m);
        Matrix operator*(const double &s);
        Matrix operator/(const Matrix &m);
        Matrix Inverse() const;
        Matrix Transpose() const;
        Matrix GetSection(const uint32_t &r1, const uint32_t &r2, const uint32_t &c1, const uint32_t &c2);

        //helpers
        void __scale_row(const uint32_t &r, const double &scale)
        {
            if(r >= dimension.first)
                return;

            for(uint32_t j=0; j<dimension.second; j++) {
                double res = (*this).at(r, j);
                res *= scale;
                if(res != 0) (*this)(r, j) = res;
            }
        }

        void __add_row_to_row(const uint32_t &r1, const uint32_t &r2)
        {
            if(r1 >= dimension.first || r2 >= dimension.first)
                return;

            for(uint32_t j=0; j<dimension.second; j++) {
                double c1 = (*this).at(r1, j);
                double c2 = (*this).at(r2, j);

                c2 = c2 + c1;
                if(c2 != 0) (*this)(r2, j) = c2;
            }
        }

        bool row_is_empty(const uint32_t &r) const
        {
            for(uint32_t j=0; j<dimension.second; j++)
            {
                if((*this).at(r, j) != 0)
                    return false;
            }
            return true;
        }

    private:
        std::pair<uint32_t, uint32_t> dimension;

        // this buffer will only save non-zero elements
        std::unordered_map<std::pair<uint32_t, uint32_t>, double, pair_hasher> _M;
    };

    std::ostream &operator<<(std::ostream& os, const Matrix &m);

};

#endif
