#ifndef GWIZ_TESTS_CLASSES_TESTVARIANTLIST_HPP
#define GWIZ_TESTS_CLASSES_TESTVARIANTLIST_HPP

#include "core/variants/IVariantList.h"

namespace gwiz
{
	namespace testing
	{

		/*
		 * This class is only for testing.
		 */

		class TestVariantList : public IVariantList
		{
		public:
			typedef std::shared_ptr< TestVariantList > SharedPtr;
		    TestVariantList(std::vector< Variant::SharedPtr > variantPtrList) :
			    m_variant_index(0),
			    m_variant_ptr_list(variantPtrList)
			{
			}

			~TestVariantList()
			{
			}

			bool getNextVariant(Variant::SharedPtr& variant) override
			{
				bool hasVariants = (this->m_variant_index < this->m_variant_ptr_list.size());
				if (hasVariants)
				{
					variant = this->m_variant_ptr_list[this->m_variant_index];
					++this->m_variant_index;
				}
				return hasVariants;
			}

			void rewind() override
			{
				this->m_variant_index = 0;
			}

			size_t size()
			{
				return this->m_variant_ptr_list.size();
			}

			IVariantList::SharedPtr getVariantsInRegion(Region::SharedPtr region) override
			{

				return nullptr;
			}

			size_t getCount() override { return m_variant_ptr_list.size(); }

		private:
			/*
			 * NOTE: Currently this only works with just one chromosome
			 */
			size_t setIndexClosestToPosition(position pos)
			{
				size_t startIndex = 0;
				size_t lastIndex = this->m_variant_ptr_list->size() - 1;
				while (startIndex <= lastIndex)
				{
					size_t midIndex = (startIndex + lastIndex) / 2;
					auto midPosition = this->m_variant_ptr_list->at(midIndex)->getLength();
					// auto midPosition = (subtractLength) ? this->m_variant_ptr_list->at(midIndex)->getPosition() - this->m_variant_ptr_list->at(midIndex)->getLength() : this->m_variant_ptr_list->at(midIndex)->getPosition();
					if (pos > midPosition)
					{
						startIndex = midIndex + 1;
					}
					else if (pos < midPosition)
					{
						lastIndex = midIndex - 1;
					}
					else
					{
						return midIndex;
					}
				}
				return lastIndex;
			}

			size_t m_variant_index;
			std::vector< Variant::SharedPtr > m_variant_ptr_list;

		};

	}
}

#endif //GWIZ_TESTS_CLASSES_TESTVARIANTLIST_HPP
