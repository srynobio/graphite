#ifndef GWIZ_IVARIANT_H
#define GWIZ_IVARIANT_H

#include <boost/noncopyable.hpp>

#include <memory>

namespace gwiz
{
    class IVariant : private boost::noncopyable
    {
        public:
            typedef std::shared_ptr<IVariant> SharedPtr;
            IVariant() {}
            virtual ~IVariant() {}

            virtual size_t getSmallestVariantSize() = 0;
            virtual size_t getLargestVariantSize() = 0;
        private:
    };
}

#endif// GWIZ_IVARIANT_H
