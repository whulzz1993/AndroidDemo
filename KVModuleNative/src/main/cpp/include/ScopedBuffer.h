//
// Created by Admin on 2020/8/10
//

#ifndef DEMO_SCOPEDBUFFER_H
#define DEMO_SCOPEDBUFFER_H
template <typename T, size_t S, bool alloc_buffer = true>
class ScopedBuffer {
        public:
        ScopedBuffer() {
            if (alloc_buffer) {
                buf_ = new T[S]();
            } else {
                buf_ = nullptr;
            }
        }

        ~ScopedBuffer() {
            if (buf_) {
                delete[] buf_;
            }
        }

        void alloc() {
            if (buf_ == nullptr) {
                buf_ = new T[S];
            }
        }

        T* buffer() const {
            return buf_;
        }

        size_t size() const {
            return S;
        }

        size_t sizeBytes() const {
            return S*sizeof(T);
        }

        private:
        T* buf_;

        DISALLOW_COPY_AND_ASSIGN(ScopedBuffer);
};
#endif //DEMO_SCOPEDBUFFER_H
