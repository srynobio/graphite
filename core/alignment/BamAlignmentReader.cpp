#include "BamAlignmentReader.h"
#include "BamAlignment.h"
#include "AlignmentList.h"
#include "SampleManager.hpp"

namespace graphite
{
	BamAlignmentReader::BamAlignmentReader(const std::string& bamPath) :
		m_bam_path(bamPath)
	{
	}

	BamAlignmentReader::~BamAlignmentReader()
	{
	}

	std::vector< IAlignment::SharedPtr > BamAlignmentReader::loadAlignmentsInRegion(Region::SharedPtr regionPtr)
	{
		if (!this->m_bam_reader.Open(this->m_bam_path))
		{
			throw "Unable to open bam file";
		}

		this->m_bam_reader.LocateIndex();
		int refID = this->m_bam_reader.GetReferenceID(regionPtr->getReferenceID());
		// add 1 to the start and end positions because this is 0 based
		this->m_bam_reader.SetRegion(refID, regionPtr->getStartPosition(), refID, regionPtr->getEndPosition());

		auto bamAlignmentPtr = std::make_shared< BamTools::BamAlignment >();
		size_t counter = 0;
		std::vector< IAlignment::SharedPtr > alignmentPtrs;
		uint32_t count = 0;
		while(this->m_bam_reader.GetNextAlignment(*bamAlignmentPtr))
		{
			if (bamAlignmentPtr->RefID != refID) { break; }
			// BamTools::SamReadGroup readGroup;
			std::string sample;
			bamAlignmentPtr->GetTag("RG", sample);

			// SampleManager::GetSample(readGroup.Sample);
			Sample::SharedPtr samplePtr = SampleManager::Instance()->getSamplePtr(sample);
			if (samplePtr == nullptr)
			{
				throw "There was an error in the sample name for: " + sample;
			}
			alignmentPtrs.push_back(std::make_shared< BamAlignment >(bamAlignmentPtr, samplePtr));
			bamAlignmentPtr = std::make_shared< BamTools::BamAlignment >();
		}
		this->m_bam_reader.Close();
		return alignmentPtrs;
	}

	std::vector< Sample::SharedPtr > BamAlignmentReader::GetBamReaderSamples(const std::string& bamPath)
	{
		std::vector< Sample::SharedPtr > samplePtrs;
		BamTools::BamReader bamReader;
		if (!bamReader.Open(bamPath))
		{
			throw "Unable to open bam file";
		}
		auto readGroups = bamReader.GetHeader().ReadGroups;
		auto iter = readGroups.Begin();
		for (; iter != readGroups.End(); ++iter)
		{
			auto samplePtr = std::make_shared< Sample >((*iter).Sample, (*iter).ID, bamPath);
			samplePtrs.emplace_back(samplePtr);
		}
		bamReader.Close();
		return samplePtrs;
	}

	position BamAlignmentReader::GetLastPositionInBam(const std::string& bamPath, Region::SharedPtr regionPtr)
	{
		BamTools::BamReader bamReader;
		if (!bamReader.Open(bamPath))
		{
			throw "Unable to open bam file";
		}

		bamReader.LocateIndex();
		int refID = bamReader.GetReferenceID(regionPtr->getReferenceID());
		auto referenceData = bamReader.GetReferenceData();
		bamReader.Close();
		return referenceData[refID].RefLength;
	}
}
