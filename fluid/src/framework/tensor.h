/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <typeindex>
#include <vector>

#include "ddim.h"
#include "data_layout.h"
#include "memory/t_malloc.h"


namespace paddle_mobile {

    namespace framework {

        class LoDTensor;

        class Tensor {
        public:
            template <typename T, size_t D, int MajorType, typename IndexType>
            friend struct EigenTensor;

            template <typename T, int MajorType, typename IndexType>
            friend struct EigenMatrix;

            template <typename T, int MajorType, typename IndexType>
            friend struct EigenVector;

        public:
            Tensor() : offset_(0) {}


            /*! Return a pointer to mutable memory block. */
            template <typename T>
            inline T* data();

            /*! Return a pointer to constant memory block. */
            template <typename T>
            inline const T* data() const;

            inline bool IsInitialized() const;


            /**
             * @brief   Return a pointer to mutable memory block.
             * @note    If not exist, then allocation.
             */
            template <typename T>
            inline T* mutable_data();

            inline void* mutable_data( std::type_index type);

            inline void* mutable_data();

            /**
             * @brief     Return a pointer to mutable memory block.
             *
             * @param[in] dims    The dimensions of the memory block.
             * @param[in] place   The place of the memory block.
             *
             * @note      If not exist, then allocation.
             */
            template <typename T>
            inline T* mutable_data(DDim dims);

            /*! Return the dimensions of the memory block. */
            inline const DDim& dims() const;

            /*! Return the numel of the memory block. */
            inline int64_t numel() const;

            /*! Resize the dimensions of the memory block. */
            inline Tensor& Resize(const DDim& dims);

            /*! The internal of two tensors share the same memory block. */
            inline Tensor& ShareDataWith(const Tensor& src);

            /**
             * @brief  Return a sub-tensor of the given tensor.
             *
             * @param[in] begin_idx   The index of the start row(inclusive) to slice.
             *                        The index number begins from 0.
             * @param[in] end_idx     The index of the end row(exclusive) to slice.
             *                        The index number begins from 0.
             */
            inline Tensor Slice(int begin_idx, int end_idx) const;

            std::type_index type() const {
//                PADDLE_ENFORCE_NOT_NULL(
//                        holder_, "Tensor not initialized yet when Tensor::type() is called.");
                return holder_->type();
            }

            // memory size returns the holding memory size in byte.
            size_t memory_size() const;

            inline void check_memory_size() const;

            inline DataLayout layout() const { return layout_; }

            inline void set_layout(const DataLayout layout) { layout_ = layout; }

        private:
            /**
             * @note    Placeholder hides type T, so it doesn't appear as a template
             *          parameter of Variable.
             */
            struct Placeholder {
                virtual ~Placeholder() = default;
                virtual void* ptr() const = 0;
                virtual size_t size() const = 0;
                virtual std::type_index type() const = 0;
//                virtual platform::Place place() const = 0;
                virtual void set_type(std::type_index type) = 0;
//                virtual void set_place(platform::Place place) = 0;
            };

            struct PlaceholderImpl : public Placeholder {
                PlaceholderImpl(size_t size, std::type_index type)
                        : ptr_(static_cast<uint8_t*>(memory::Alloc(size)),
                               memory::PODDeleter<uint8_t>()),
                          size_(size),
                          type_(type) {
//                    PADDLE_ENFORCE_NOT_NULL(ptr_, "Insufficient %s memory to allocation.",
//                                            (is_cpu_place(place_) ? "CPU" : "GPU"));
                }

                virtual size_t size() const { return size_; }
                virtual void* ptr() const { return static_cast<void*>(ptr_.get()); }
                virtual std::type_index type() const { return type_; }
                virtual void set_type(std::type_index type) { type_ = type; }

                /*! the pointer of memory block. */
                std::unique_ptr<uint8_t, memory::PODDeleter<uint8_t>> ptr_;

                /*! the size of memory block. */
                size_t size_;

                /* the current type of memory */
                std::type_index type_;
            };

            /*! holds the memory block if allocated. */
            std::shared_ptr<Placeholder> holder_;

            /**
             * @brief points to elements dimensions.
             *
             * @note dims_ do not indicate the memory block size.
             */

            DDim dims_;

            /**
             * @brief the layout of memory block, default is NHWC.
             *
             * @note the memory allocation order, describe how weight/data is stored
             *       For example, in 4-D Tensor(rank=4), there are three commonly
             *       used layout. They are
             *            NCHW, NHWC, CHWN.
             *       N,C,H,W for respectively the batch size, the number of
             *       feature maps, the height.
             */

            DataLayout layout_ = DataLayout::kNHWC;

            /**
             * @brief   A PlaceHolder may be shared by more than one tensor.
             *
             * @note    Some of them may be slices of the others. So the offset_
             *          is introduced here to indicate the byte offset between
             *          PlaceHolder::ptr_ and where the tensor data really begins.
             */
            size_t offset_;
        };

    }  // namespace framework
}  // namespace paddle

