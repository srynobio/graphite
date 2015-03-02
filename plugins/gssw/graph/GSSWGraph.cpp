#include "GSSWGraph.h"
#include "vg/graph/ReferenceNode.h"
#include "vg/graph/SNPNode.h"
#include "vg/graph/IVariantNode.h"

#include <map>

namespace gwiz
{
namespace gssw
{
	GSSWGraph::GSSWGraph(gwiz::IReference::SharedPtr referencePtr, gwiz::IVariantList::SharedPtr variantListPtr, IAlignmentReader::SharedPtr alignmentReader) :
		IGraph(referencePtr, variantListPtr), m_alignment_reader(alignmentReader), m_match(2), m_mismatch(2), m_gap_open(3), m_gap_extension(1)

	{
		this->m_nt_table = gssw_create_nt_table();
		this->m_mat = gssw_create_score_matrix(this->m_match, this->m_mismatch);
		this->m_graph_ptr = gssw_graph_create(100);
	}

	GSSWGraph::~GSSWGraph()
	{
		gssw_graph_destroy(this->m_graph_ptr);
		free(this->m_nt_table);
		free(this->m_mat);
	}

	void GSSWGraph::constructGraph()
	{
		size_t readLength = m_alignment_reader->getAverageReadLength();
		position startPosition = this->m_reference_ptr->getRegion()->getStartPosition();
		size_t referenceOffset = 0;
		size_t referenceSize;
		Variant::SharedPtr variantPtr;
		std::vector< gssw_node* > altAndRefVertices;
		size_t graphSize = 0;
		int count = 0;

		if (false)
		{
			auto referenceNode = gssw_node_create_alt(this->m_reference_ptr->getSequence() + referenceOffset, this->m_reference_ptr->getSequenceSize(), this->m_nt_table, this->m_mat);
			gssw_graph_add_node(this->m_graph_ptr, referenceNode);
			graphConstructed();
			return;
		}
		while (getNextCompoundVariant(variantPtr))
		{
			referenceSize = variantPtr->getPosition() - (startPosition + referenceOffset);
			if (referenceSize > 0)
			{
				auto referenceNode = gssw_node_create_alt(this->m_reference_ptr->getSequence() + referenceOffset, referenceSize, this->m_nt_table, this->m_mat);
			    addReference(altAndRefVertices, referenceNode);
				altAndRefVertices.clear();
				altAndRefVertices.push_back(referenceNode);
			}
			size_t variantReferenceSize;
			altAndRefVertices = addVariantVertices(altAndRefVertices, variantPtr, variantReferenceSize);
			referenceOffset += referenceSize + variantReferenceSize;
		}
		referenceSize = this->m_reference_ptr->getRegion()->getEndPosition() - (startPosition - referenceOffset);
		if (referenceSize > 0)
		{
			auto referenceNode = gssw_node_create_alt(this->m_reference_ptr->getSequence() + referenceOffset, referenceSize, this->m_nt_table, this->m_mat);
				addReference(altAndRefVertices, referenceNode);
		}
		graphConstructed();
	}

	gssw_node* GSSWGraph::addReference(std::vector< gssw_node* > altAndRefVertices, gssw_node* referenceNode)
	{
		gssw_graph_add_node(this->m_graph_ptr, referenceNode);
		for (auto iter = altAndRefVertices.begin(); iter != altAndRefVertices.end(); ++iter)
		{
			gssw_nodes_add_edge((*iter), referenceNode);
		}
		return referenceNode;
	}

	std::vector< gssw_node* > GSSWGraph::addVariantVertices(std::vector< gssw_node* > altAndRefVertices, Variant::SharedPtr variantPtr, size_t& variantReferenceSize)
	{
	    std::vector< gssw_node* > vertices;
		for (uint32_t i = 0; i < variantPtr->getAlt().size(); ++i)
		{
			INode::SharedPtr variantNodePtr = vg::IVariantNode::BuildVariantNodes(variantPtr, i);
			// gssw_node* variantNode = gssw_node_create_alt(variantNodePtr->getSequence(), variantNodePtr->getLength(), this->m_nt_table, this->m_mat);
			// gssw_graph_add_node(this->m_graph_ptr, variantNode);
			vertices.push_back(addGSSWVariantNode(variantNodePtr));
		}
		size_t referenceOffset = variantPtr->getPosition() - this->m_reference_ptr->getRegion()->getStartPosition();
		gssw_node* variantReferenceNode = gssw_node_create_alt(this->m_reference_ptr->getSequence() + referenceOffset, variantPtr->getRef()[0].size(), this->m_nt_table, this->m_mat);
		gssw_graph_add_node(this->m_graph_ptr, variantReferenceNode);
		vertices.push_back(variantReferenceNode);
		variantReferenceSize = variantPtr->getRef()[0].size();
		for (auto parentVertexIter = altAndRefVertices.begin(); parentVertexIter != altAndRefVertices.end(); ++parentVertexIter)
		{
			for (auto childVertexIter = vertices.begin(); childVertexIter != vertices.end(); ++childVertexIter)
			{
				gssw_nodes_add_edge((*parentVertexIter), (*childVertexIter));
			}
		}
		return vertices;
	}

	bool GSSWGraph::recenterGraph(IAlignment::SharedPtr alignment)
	{
		return false;
	}

	void GSSWGraph::graphConstructed()
	{
	std::cout << "graphConstructed" << std::endl;
		IAlignment::SharedPtr alignmentPtr;
		while (this->m_alignment_reader->getNextAlignment(alignmentPtr))
		{
			std::string readSeq = std::string(alignmentPtr->getSequence(), alignmentPtr->getLength());
			gssw_graph_fill(this->m_graph_ptr, readSeq.c_str(), this->m_nt_table, this->m_mat, this->m_gap_open, this->m_gap_extension, 15, 2);
			gssw_graph_mapping* gm = gssw_graph_trace_back (this->m_graph_ptr,
															readSeq.c_str(),
															readSeq.size(),
															m_match,
															m_mismatch,
															m_gap_open,
															m_gap_extension);
			std::cout << std::string(alignmentPtr->getSequence(), alignmentPtr->getLength()) << std::endl;
			// std::cout << "Position: " << alignmentPtr->getPosition() << std::endl;
			gssw_print_graph_mapping(gm);
			// gssw_node* node = gm->cigar.elements->node;

			std::cout << "elements: " << gm->cigar.length;
			/*
			do
			{
				std::cout << std::string(node->seq, node->len) << "|||";
				// std::cout << gm->cigar.elements[i].node->seq << "|||";
				node = node->next;
			} while (node != NULL);
			*/
			std::cout << std::endl << std::endl;
			gssw_graph_mapping_destroy(gm);
		}
	}
}
}