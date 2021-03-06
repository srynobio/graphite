#ifndef GRAPHITE_ADJUDICATIONTESTS_HPP
#define GRAPHITE_ADJUDICATIONTESTS_HPP

#include "TestConfig.h"

#include "core/reference/IReference.h"
#include "core/util/ThreadPool.hpp"
#include "core/variant/IVariantList.h"
#include "core/variant/IVariant.h"
#include "core/adjudicator/IAdjudicator.h"
#include "core/mapping/MappingManager.h"

#include "plugins/adjudicator/graph/GSSWGraph.h"
#include "plugins/adjudicator/graph/GSSWAdjudicator.h"
#include "plugins/adjudicator/graph/GSSWMapping.h"


#include <vector>

namespace
{
namespace adj_test
{
	using namespace graphite;
	using namespace graphite::adjudicator;

	class AdjudicationTest : public ::testing::Test
	{
		virtual void TearDown()
		{
			// clear all mappings after the test
			MappingManager::Instance()->clearRegisteredMappings();
		}
	};

	class ReferenceTest : public IReference
	{
	public:
        ReferenceTest(Region::SharedPtr regionPtr, const char* sequence) : m_sequence(sequence) { this->m_region = regionPtr; }
		~ReferenceTest() {}

		const char* getSequence() override
		{
			return m_sequence;
		}

		size_t getSequenceSize() override
		{
			return strlen(m_sequence);
		}

	private:
		const char* m_sequence;
	};

	class VariantListTest : public IVariantList
	{
	public:
		VariantListTest(std::vector< IVariant::SharedPtr > variants) : m_variant_list(variants), m_index(0) {}
		~VariantListTest() {}

		bool getNextVariant(IVariant::SharedPtr& variantPtr) override
		{
			if (this->m_variant_list.size() <= this->m_index) { variantPtr = nullptr; return false; }
			else { variantPtr = this->m_variant_list[this->m_index++]; return true; }
		}

		size_t getCount() override { return this->m_variant_list.size(); }
		void sort() override {}
		void printToVCF(std::ostream& out) {}

	private:
		std::vector< IVariant::SharedPtr > m_variant_list;
		uint32_t m_index;
	};

	class AlignmentTest : public IAlignment
	{
	public:
		AlignmentTest(const std::string sequence, position pos) : m_sequence(sequence), m_position(pos)
		{
		}
		~AlignmentTest() {}

		const char* getSequence() override { return this->m_sequence.c_str(); }
		const size_t getLength() override { return this->m_sequence.size(); }
		const position getPosition() override { return this->m_position; }
		const void setSequence(char* seq, uint32_t len) override {}
		const void removeSequence() override {}
	private:
		std::string m_sequence;
		position m_position;
	};

		GSSWGraph::SharedPtr getGSSWGraph(IReference::SharedPtr referencePtr, IVariantList::SharedPtr variantListPtr, IAdjudicator::SharedPtr adjudicatorPtr, uint32_t graphSize = 3000, uint32_t startPosition = 1)
	{
		// uint32_t startPosition = 1;
		// uint32_t graphSize = 3000;
		auto gsswGraphPtr = std::make_shared< GSSWGraph >(referencePtr, variantListPtr, startPosition, graphSize, adjudicatorPtr->getMatchValue(), adjudicatorPtr->getMisMatchValue(), adjudicatorPtr->getGapOpenValue(), adjudicatorPtr->getGapExtensionValue());
		gsswGraphPtr->constructGraph();
		return gsswGraphPtr;
	}

	TEST_F(AdjudicationTest, AdjudicateDualSNPMatch)
	{
		ThreadPool::Instance()->setThreadCount(1);
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");
		auto refAllelePtr = std::make_shared< Allele >("A");
		auto altAllelePtr = std::make_shared< Allele >("G");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		position pos = 10;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("CTCAAGTAGAATCTACTCTCTCAGGTGTTCATAATGTATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCTT", 2);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateDualSNPMisMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");
		auto refAllelePtr = std::make_shared< Allele >("A");
		auto altAllelePtr = std::make_shared< Allele >("C");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		position pos = 10;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("CTCAAGTAGAATCTACTCTCTCAGGTGTTCATAATGTATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCTT", 2);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
		// ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateDuoSNPMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");
		auto refAllelePtr = std::make_shared< Allele >("C");
		auto altAllelePtr = std::make_shared< Allele >("G");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		position pos = 17;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		size_t alignmentOffset = 9;
		size_t alignmentSize = 150;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >(std::string(seq.c_str() + alignmentOffset, alignmentSize), alignmentOffset);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateTriSNPMisMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");
		auto refAllelePtr = std::make_shared< Allele >("T");
		auto altAllele1Ptr = std::make_shared< Allele >("G");
		auto altAllele2Ptr = std::make_shared< Allele >("A");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr, altAllele2Ptr };
		position pos = 15;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		size_t alignmentOffset = 9;
		size_t alignmentSize = 150;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >(std::string(seq.c_str() + alignmentOffset, alignmentSize), alignmentOffset);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 0);
		ASSERT_EQ(altAllele2Ptr->getTotalCount(), 0);
		ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateTriSNPMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");
		auto refAllelePtr = std::make_shared< Allele >("T");
		auto altAllele1Ptr = std::make_shared< Allele >("G");
		auto altAllele2Ptr = std::make_shared< Allele >("A");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr, altAllele2Ptr };
		position pos = 15;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("AAATCAACTCTCTCAGGTGTTCATAATGTATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCTTATTCCTTC", 6);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 0);
		ASSERT_EQ(altAllele2Ptr->getTotalCount(), 1);
		ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateShortDeletionMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");
		auto refAllelePtr = std::make_shared< Allele >("AGGTGTTCATAATGT");
		auto altAllele1Ptr = std::make_shared< Allele >("AT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr };
		position pos = 24;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("TACTCTCTCATATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCTTATTCCTTCCCCTG", 6);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		// this is why you aren't deleting mappings
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 1);
		ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateShortInsertionMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");

		auto refAllelePtr = std::make_shared< Allele >("A");
		auto altAllele1Ptr = std::make_shared< Allele >("AGGGGGGGGGG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr };
		position pos = 5;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("ACTCAGGGGGGGGGGAGTAAAATCTACTCTCTCAGGTGTTCATAATGTATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCT", 1);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 1);
		ASSERT_EQ(gsswMappingPtr->getMappingScore(), (alignmentPtr->getLength() * match));
	}

	TEST_F(AdjudicationTest, AdjudicateShortInsertionMultipleMatch)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");

		auto refAllelePtr = std::make_shared< Allele >("A");
		auto altAllele1Ptr = std::make_shared< Allele >("AGGGGGGGGGG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr };
		position pos = 5;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("ACTCAGGGGGGGGGGAGTAAAATCTACTCTCTCAGGTGTTCATAATGTATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCT", 1);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateDualIdenticalIntoVariant)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");

		auto refAllelePtr = std::make_shared< Allele >("TTCCCCTGACCCT");
		auto altAllele1Ptr = std::make_shared< Allele >("TTCCCCTGTTTTATCTTTT");
		auto altAllele2Ptr = std::make_shared< Allele >("TTCCGTTTTATCTTTT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr, altAllele2Ptr };
		position pos = 157;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		variantPtr->processOverlappingAlleles();
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("GTTTTATCTTTTATTCTCCAAATTTCTTTCCAATTCATCTTTGTTCTTCCCTTTCCTTTTTACTCTCTTTAAACATTCTATGGACTCTGCCTCCTTCACACTGATATTGAACGCCCATAGTTTCATATTTTGGATTGCGATTGTTTTATT", 1);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 0);
		ASSERT_EQ(altAllele2Ptr->getTotalCount(), 0);
	}

	TEST_F(AdjudicationTest, AdjudicateDualIdenticalOutOfVariant)
	{
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");

		auto refAllelePtr = std::make_shared< Allele >("TGACCCTTCTTTTATTCTC");
		auto altAllele1Ptr = std::make_shared< Allele >("TGACCCTTACCTG");
		auto altAllele2Ptr = std::make_shared< Allele >("TGACCCTTGATT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllele1Ptr, altAllele2Ptr };
		position pos = 163;
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		variantPtr->processOverlappingAlleles();
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr);
		auto alignmentPtr = std::make_shared< AlignmentTest >("CTCAGGTGTTCATAATGTATCAATGTATATTGCTTTAAGCCTGAAGGTAACCTAAGTAAAGATGTACCATGTTCCACCAATGCTTCTTTTGATCATCATTTTATCCTGTTTTTTCTTTAGGATTCTTTCTTATTCCTTCCCCTGACCCTT", 1);

		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		// gsswMappingPtr->printLongFormat();
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 0);
		ASSERT_EQ(altAllele2Ptr->getTotalCount(), 0);
	}

	TEST_F(AdjudicationTest, AdjudicateRemapPreviouslyMappedAlignment)
	{
		ThreadPool::Instance()->setThreadCount(1);
		std::string seq = TEST_REFERENCE_SEQUENCE;
		auto regionPtr = std::make_shared< Region >("1:1-3720");

		auto refAllele1Ptr = std::make_shared< Allele >("C");
		auto altAllele1Ptr = std::make_shared< Allele >("G");
		std::vector< IAllele::SharedPtr > altAllele1Ptrs = { altAllele1Ptr };
		position pos1 = 17;
		std::string chrom = "1";
		std::string dot = ".";
		auto variant1Ptr = std::make_shared< Variant >(pos1, chrom, dot, dot, dot, refAllele1Ptr, altAllele1Ptrs);

		auto refAllele2Ptr = std::make_shared< Allele >("TGAC");
		auto altAllele2Ptr = std::make_shared< Allele >("ATCTATTCTCTCAGG");
		std::vector< IAllele::SharedPtr > altAllele2Ptrs = { altAllele2Ptr };
		position pos2 = 3503;
		auto variant2Ptr = std::make_shared< Variant >(pos2, chrom, dot, dot, dot, refAllele2Ptr, altAllele2Ptrs);

		std::vector< IVariant::SharedPtr > variant1Ptrs = { variant1Ptr };
		std::vector< IVariant::SharedPtr > variant2Ptrs = { variant2Ptr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantList1Ptr = std::make_shared< VariantList >(variant1Ptrs, referencePtr);
		auto variantList2Ptr = std::make_shared< VariantList >(variant2Ptrs, referencePtr);

		uint32_t percent = 50;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraph1Ptr = getGSSWGraph(referencePtr, variantList1Ptr, gsswAdjudicatorPtr, 1000, 1);
		auto gsswGraph2Ptr = getGSSWGraph(referencePtr, variantList2Ptr, gsswAdjudicatorPtr, 3000, 500);
		auto alignmentPtr = std::make_shared< AlignmentTest >("AGTAAAATCTATTCTCTCAGGT", 2);

		variantList1Ptr->processOverlappingAlleles();
		variantList2Ptr->processOverlappingAlleles();

		auto gsswMapping1Ptr = std::make_shared< GSSWMapping >(gsswGraph1Ptr->traceBackAlignment(alignmentPtr), alignmentPtr);
		auto gsswMapping2Ptr = std::make_shared< GSSWMapping >(gsswGraph2Ptr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMapping1Ptr);
		MappingManager::Instance()->registerMapping(gsswMapping2Ptr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();

		ASSERT_EQ(refAllele1Ptr->getTotalCount(), 0);
		ASSERT_EQ(altAllele1Ptr->getTotalCount(), 0);
		ASSERT_EQ(refAllele2Ptr->getTotalCount(), 0);
		ASSERT_EQ(altAllele2Ptr->getTotalCount(), 1);

	}

	void adjudicateAlleles(const std::string& seq, position pos, Allele::SharedPtr refAllelePtr, std::vector< IAllele::SharedPtr > altAllelePtrs, IAlignment::SharedPtr alignmentPtr)
	{
		auto regionPtr = std::make_shared< Region >("1:0-" + std::to_string(seq.size()));
		std::string chrom = "1";
		std::string dot = ".";
		auto variantPtr = std::make_shared< Variant >(pos, chrom, dot, dot, dot, refAllelePtr, altAllelePtrs);
		variantPtr->processOverlappingAlleles();
		std::vector< IVariant::SharedPtr > variantPtrs = { variantPtr };
		auto referencePtr = std::make_shared< ReferenceTest >(regionPtr, seq.c_str());
		auto variantListPtr = std::make_shared< VariantList >(variantPtrs, referencePtr);

		uint32_t percent = 80;
		int match = 1;
		int mismatch = 4;
		int gapOpen = 6;
		int gapExtension = 1;
		auto gsswAdjudicatorPtr = std::make_shared< GSSWAdjudicator >(percent, match, mismatch, gapOpen, gapExtension);
		auto gsswGraphPtr = getGSSWGraph(referencePtr, variantListPtr, gsswAdjudicatorPtr, seq.size(), 0);
		variantListPtr->processOverlappingAlleles();

		auto gsswMappingPtr = std::make_shared< GSSWMapping >(gsswGraphPtr->traceBackAlignment(alignmentPtr), alignmentPtr);
		MappingManager::Instance()->registerMapping(gsswMappingPtr);
		MappingManager::Instance()->evaluateAlignmentMappings(gsswAdjudicatorPtr);
		ThreadPool::Instance()->joinAll();
	}

		/*
		  The key to the test names are
		  Adjudicate{quality}{type}Match{W/P/N/A}
		  W = whole node match
		  P = partial node match
		  N = does not go through node
		  A = ambiguous, no call
		 */
	TEST_F(AdjudicationTest, AdjudicateExactReferenceMatchWWW)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("ATACGTTTACGCTTACGT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
	}

	TEST_F(AdjudicationTest, AdjudicateExactRefMatchWWP)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("ATACGTTTACG", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
	}

	TEST_F(AdjudicationTest, AdjudicateExactRefMatchPWW)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("TTTACGCTTACGT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
	}

	TEST_F(AdjudicationTest, AdjudicateExactRefMatchPWP)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("TTTACG", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 1);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
	}

    TEST_F(AdjudicationTest, AdjudicateExactAltMatchWWW)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("ATACGTTAGCTTACGT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchWWP)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("ATACGTTAGC", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchPWP)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("TTAGC", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchPWW)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("TTAGCTTACGT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchWPN)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("TTA", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchNPW)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TAG");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("TTAGCTTACGT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchNWN)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("AGGGACCTAGGCT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("AGGGACCTAGGCT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAltMatchNPN)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("AGGGACCTAGGCT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("GGACCTAGG", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 1);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAmbiguousWAN)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACG");
		auto altAllelePtr = std::make_shared< Allele >("TTACT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("ATACGTTTAC", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
	}

	TEST_F(AdjudicationTest, AdjudicateExactAmbiguousNAW)
	{
		std::string seq = "ATACGTTTACGCTTACGT";
		auto refAllelePtr = std::make_shared< Allele >("TTACGCT");
		auto altAllelePtr = std::make_shared< Allele >("TGGCGCT");
		std::vector< IAllele::SharedPtr > altAllelePtrs = { altAllelePtr };
		auto alignmentPtr = std::make_shared< AlignmentTest >("CGCTTACGT", 0);
		adjudicateAlleles(seq, 6, refAllelePtr, altAllelePtrs, alignmentPtr);

		ASSERT_EQ(refAllelePtr->getTotalCount(), 0);
		ASSERT_EQ(altAllelePtr->getTotalCount(), 0);
	}
}
}

#endif //GRAPHITE_ADJUDICATIONTESTS_HPP
