#ifndef GWIZ_VG_VARIANT_GRAPH_TESTS_HPP
#define GWIZ_VG_VARIANT_GRAPH_TESTS_HPP
#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "config/TestConfig.h"
#include "tests/classes/TestReference.hpp"
#include "tests/classes/TestVariantList.hpp"
#include "tests/classes/TestReferenceVariantGenerator.hpp"
#include "plugins/vg/graph/VariantGraph.h"

#include "core/variants/VCFFileReader.h"
#include "core/variants/IVariant.h"
#include "core/reference/FastaReference.h"

namespace
{

	class VGTest : public gwiz::vg::VariantGraph
	{
	public:
		typedef std::shared_ptr< VGTest > VGTestPtr;

		VGTest(gwiz::IReference::SharedPtr referencePtr, gwiz::IVariantList::SharedPtr variantListPtr)
		{
			m_next_variant_init = false;
			m_reference_ptr = referencePtr;
			m_variant_list_ptr = variantListPtr;
			m_graph_ptr = std::make_shared< gwiz::vg::VariantGraph::Graph >();
		}

		~VGTest()
		{
		}

		void constructGraph() override
		{
			constructGraph();
		}

		bool getNextCompoundVariant(gwiz::Variant::SharedPtr& variant) override
		{
			return gwiz::vg::VariantGraph::getNextCompoundVariant(variant);
		}

		gwiz::Variant::SharedPtr buildCompoundVariant(const std::string& referenceString, const std::vector< gwiz::Variant::SharedPtr >& variants) override
		{
			return gwiz::vg::VariantGraph::buildCompoundVariant(referenceString, variants);
		}
	};

	//std::string m_reference_string = "GGCCTCAAGTTATCCACCCACCTTGGCCTCCCAAAGCGCTGAGATTACAGGTGTGAACCACTGCACCTGGTCTCAAATCAGAAAATCTTTGAATTCATCTAATGGTCACCTATCCTGTGGGCCCTCACTTTGAGATATTTTGCCTTTTTTGGCCAAACCAATATGTAGCCTCCATGTATTATATGACCTTGCCTGCAACCTCTGCCTTCCCACCTTTAAAAACCCTTACACATAAGCCATCAGGGAGATTAGGCCTTAAGGATTAGCTGCCTGATACTCCTTGCTTGCTGCCTGCAATAAATTCCTCAACTTCTGTCTCAGCAATGCCGATATCAGTGCTTGACTTTGATAGGCTGGGTGGGTGGACCCAAATTTGGTTTGGTGACCCTTTGAGCTTAGATTCAAAATTCTAGTTTTGTCACTCTGCAGTTTTGTGATCTTGAGCAAGTTACTTAACCTCTCTGAGCCTTGTTTGTCATGTGTAATGAAAAGAGCTATACTTACCTTGTGAGGTAGTCCTCAGGATTCAATGAGATAATAAGTACTCACTAAACAAAACTCGTTATTACAAAAGAATCACTTTGTCTCTGAAGTGGGCAATTCAACCCATTTCTAGGAGATTTTAAACATGATTTTAGATATTTGGTGTGATTTTGTGAATGGGTTTATCGTTAATAGCTTTCATGCTCCAGAATTTTCTTGAATAATAGGTTTTTGCAAAGTGCATTCCATGGAATACTCATTTGGGTGACGTTAATAGACATCACTCAAAAGCTGGGTGAATATTACAATGTTTACTTCATCTGTAACAAGCTGAGTAGCTACAGTACATATCTAAGAGGGGGCTCTAATTCTCAATATTTTCCAAATTTATTAGATCACAGACTTTTCTTTTAGTGAAGTGCTTAATGAAACTTAAGTTCTGTGAAAAGTACTTTGAGAAATATTGCTTTAAAAAGAAAAAGATTGAGCCCTGTATCAGGGGAAATATCTAATATTATATTAAACAAAAAAGTCCCA";
	const std::string m_reference_string = "AAATAAAAGTTATCCACCCACCTTGGCCTCCCAAAGCGCTG";


	TEST(VariantGraphTest, getCompoundVariantEmptyTest)
	{
		gwiz::position pos = 6000;
		gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", pos);

		auto variantGraph = std::make_shared< VGTest >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());
		gwiz::Variant::SharedPtr variantPtr;
		variantGraph->getNextCompoundVariant(variantPtr);
		ASSERT_TRUE(variantPtr == NULL);
	}


	TEST(VariantGraphTest, getCompoundVariantSingleTest)
	{
		gwiz::position pos = 6000;
		gwiz::position variantPositionOffset = 3;
		gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", pos);
        //AAATAAAAGTTATCCACCCACCTTGGCCTCCCAAAGCGCTG
		testReferenceVariantGenerator.addVariant(pos + variantPositionOffset, ".", 1, {"T"});

		auto variantGraph = std::make_shared< VGTest >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());

		gwiz::Variant::SharedPtr variantPtr;
		variantGraph->getNextCompoundVariant(variantPtr);
		ASSERT_STREQ(variantPtr->getRef()[0].c_str(), std::string(m_reference_string.c_str() + variantPositionOffset, 1).c_str());

		variantGraph->getNextCompoundVariant(variantPtr);
		ASSERT_TRUE(variantPtr == NULL);
	}

	TEST(VariantGraphTest, getCompoundVariantSNPSinglePositionTest)
	{
		gwiz::position pos = 6000;
		gwiz::position variantPositionOffset = 3;
		gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", pos);
        //AAATAAAAGTTATCCACCCACCTTGGCCTCCCAAAGCGCTG
		testReferenceVariantGenerator.addVariant(pos + variantPositionOffset, "1", 1, {"A"});
		testReferenceVariantGenerator.addVariant(pos + variantPositionOffset, "2", 1, {"G"});

		auto variantGraph = std::make_shared< VGTest >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());
		gwiz::Variant::SharedPtr variantPtr;
		variantGraph->getNextCompoundVariant(variantPtr);
		ASSERT_STREQ(variantPtr->getAlt()[0].c_str(), "A");
		ASSERT_STREQ(variantPtr->getAlt()[1].c_str(), "G");
	}

	TEST(VariantGraphTest, getCompoundVariantMultiSinglePositionTest)
	{
		gwiz::position pos = 6000;
		gwiz::position variantPositionOffset = 3;
		gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", pos);
        //AAATAAAAGTTATCCACCCACCTTGGCCTCCCAAAGCGCTG
		testReferenceVariantGenerator.addVariant(pos + variantPositionOffset, "1", 2, {"AC"});
		testReferenceVariantGenerator.addVariant(pos + variantPositionOffset, "2", 1, {"G"});

		auto variantGraph = std::make_shared< VGTest >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());

		gwiz::Variant::SharedPtr variantPtr;
		variantGraph->getNextCompoundVariant(variantPtr);

		ASSERT_STREQ(variantPtr->getAlt()[0].c_str(), "AC");
		ASSERT_STREQ(variantPtr->getAlt()[1].c_str(), "GA");
	}

	/*

	TEST_F(VariantGraphTest, ConstructGraphNoVariants)
	{
		gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", 6000);
		// auto variantGraph = std::make_shared< gwiz::vg::VariantGraph >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());
		auto variantGraph = std::make_shared< VGTest >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());
	}

	TEST_F(VariantGraphTest, MultipleVariantsSequentially)
	{
		gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", 6000);
        //AAATAAAAGTTATCCACCCACCTTGGCCTCCCAAAGCGCTG
		testReferenceVariantGenerator.addVariant(6001, ".", 1, {"T"});
		testReferenceVariantGenerator.addVariant(6002, ".", 1, {"G"});
		testReferenceVariantGenerator.addVariant(6003, ".", 1, {"A"});
		testReferenceVariantGenerator.addVariant(6004, ".", 1, {"C"});
		testReferenceVariantGenerator.addVariant(6005, ".", 1, {"G"});
		// testReferenceVariantGenerator.addVariant(6015, ".", 1, {"AAAC","GG"});
		// testReferenceVariantGenerator.addVariant(6020, ".", 1, {"A","CCC"});

		auto variantGraph = std::make_shared< gwiz::vg::VariantGraph >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());
		variantGraph->printGraph("test.dot");
	}
	*/


	/*
	TEST_F(VariantGraphTest, ConstructGraphYChr)
	{

		// gwiz::Region::SharedPtr regionPtr = std::make_shared< gwiz::Region >("20:60343-62965354");
		//gwiz::Region::SharedPtr regionPtr = std::make_shared< gwiz::Region >("Y:2655180-2656128");
		// problem is at position: 2820410
		gwiz::Region::SharedPtr regionPtr = std::make_shared< gwiz::Region >("Y:2820400-2820410");

		std::string fastaPath = TEST_FASTA_FILE;
		std::string vcfPath = TEST_1KG_CHRY_VCF_FILE;
		// std::string vcfPath = TEST_1KG_VCF_FILE;

		auto fastaReferencePtr = std::make_shared< gwiz::FastaReference >(fastaPath, regionPtr);
		auto vcfFileReader = std::make_shared<gwiz::VCFFileReader>(vcfPath, regionPtr);
		auto variantGraph = std::make_shared< gwiz::vg::VariantGraph >(fastaReferencePtr, vcfFileReader);
		variantGraph->printGraph("test1.dot");


		// gwiz::testing::TestReferenceVariantGenerator testReferenceVariantGenerator(m_reference_string, "20", 6000);
		// testReferenceVariantGenerator.addVariant(6010, ".", {"A","C"});
		// auto variantGraph = std::make_shared< gwiz::vg::VariantGraph >(testReferenceVariantGenerator.getReference(), testReferenceVariantGenerator.getVariants());

	}
	*/

}

#endif //VARIANT_GRAPH_TESTS_HPP
