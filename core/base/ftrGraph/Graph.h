/// \ingroup base
/// \class ttk::ftr::Graph
/// \author Gueunet Charles <charles.gueunet+ttk@gmail.com>
/// \date 2018-01-25
///
/// \brief TTK %FTRGraph graph skeleton
///
/// This class manage nodes and arcs of the graph structure
/// along with their Segmentation
///
/// \sa ttk::FTRGraph

#pragma once

#include "AtomicVector.h"
#include "DataTypesFTR.h"
#include "FTRCommon.h"
#include "Node.h"
#include "SuperArc.h"
#include "Scalars.h"
#include "Propagation.h"

#ifndef TTK_ENABLE_KAMIKAZE
#include<iostream>
#endif

#include <forward_list>
#include <vector>

namespace ttk
{
   namespace ftr
   {

      class Graph : public Allocable
      {
        private:
         AtomicVector<idVertex> leaves_;
         AtomicVector<Node>     nodes_;
         AtomicVector<SuperArc> arcs_;

         std::vector<std::forward_list<idSuperArc>> segmentation_;
         std::vector<valence>                     valences_;

        public:
         Graph();
         Graph(Graph&& other)      = default;
         Graph(const Graph& other) = delete;
         virtual ~Graph();

         Graph& operator=(Graph&& other) {
            if (this != &other) {
               leaves_       = std::move(other.leaves_);
               nodes_        = std::move(other.nodes_);
               arcs_         = std::move(other.arcs_);
               segmentation_ = std::move(other.segmentation_);
               valences_     = std::move(other.valences_);
            }
            return *this;
         }

         Graph& operator=(Graph& other) = delete;

         // Accessor on structure
         // ---------------------

         idNode getNumberOfNodes(void) const
         {
            return nodes_.size();
         }

         idNode getNumberOfLeaves(void) const
         {
            return leaves_.size();
         }

         idVertex getLeaf(const idNode id) const
         {
            return leaves_[id];
         }

         const Node& getNode(const idNode id) const
         {
           return nodes_[id];
         }

         const SuperArc& getArc(const idSuperArc id) const
         {
            return arcs_[id];
         }

         bool isVisited(const idVertex v) const
         {
            return !segmentation_[v].empty();
         }

         void visit(const idVertex v, const idSuperArc id)
         {
            segmentation_[v].emplace_front(id);
         }

         const std::forward_list<idSuperArc> visit(const idVertex v) const
         {
            return segmentation_[v];
         }

         bool hasVisited(const idVertex v, const idSuperArc id) const
         {
            for(const idSuperArc tmp :  segmentation_[v]){
               if (tmp == id) return true;
            }
            return false;
         }

         idSuperArc getFirstVisit(const idVertex v) const
         {
            return segmentation_[v].front();
         }

#ifndef TTK_ENABLE_KAMIKAZE
         valence getNbVisit(const idVertex v) const
         {
            // Debug only, costly
            valence s = 0;
            for(const idSuperArc tmp :  segmentation_[v]){
               std::ignore = tmp;
               ++s;
            }
            return s;
         }
#endif

         // direct access for openmp
         const valence& val(const idVertex v) const
         {
            return valences_[v];
         }

         valence& val(const idVertex v)
         {
            return valences_[v];
         }

         // Build structure
         // ---------------

         void addLeaf(const idVertex v)
         {
            leaves_.emplace_back(v);
         }

         idNode makeNode(const idVertex v)
         {
            const idNode newNode = nodes_.getNext();
            nodes_[newNode].setVerterIdentifier(v);
            return newNode;
         }

         idSuperArc openArc(const idNode downId, Propagation * p = nullptr)
         {
             idSuperArc newArc = arcs_.getNext();
             arcs_[newArc].setDownNodeId(downId);
             if (p) {
                arcs_[newArc].setPropagation(p);
             }
             return newArc;
         }

         void closeArc(const idSuperArc arc, const idNode upId)
         {
            arcs_[arc].setUpNodeId(upId);
         }

         void makeArc(const idNode downId, const idNode upId)
         {
            arcs_.emplace_back(SuperArc{downId, upId});
         }

         // Process
         // -------

         template<typename ScalarType>
         void sortLeaves(const Scalars<ScalarType>* s, const bool parallel = false){
            auto compare_fun = [&](idVertex a, idVertex b) { return s->isLower(a, b); };
            if(parallel){
               ::ttk::ftr::parallel_sort<decltype(leaves_.begin()), idVertex>(
                   leaves_.begin(), leaves_.end(), compare_fun);
            } else {
               ::ttk::ftr::sort<decltype(leaves_.begin()), idVertex>(leaves_.begin(), leaves_.end(),
                                                                     compare_fun);
            }
         }

         // Merge porpagations and close arcs at a saddle
         void mergeAtSaddle(const idNode saddleId)
         {
            const idVertex saddleVert = getNode(saddleId).getVertexIdentifier();
#ifndef TTK_ENABLE_KAMIKAZE
            if (getNbVisit(saddleVert) < 2) {
               std::cerr << "[FTR Graph]: merge on saddle having less than 2 visits:";
               std::cerr << saddleVert << std::endl;
            }
#endif
            const idSuperArc firstArc  = getFirstVisit(saddleVert);
            Propagation*     firstProp = getArc(firstArc).getPropagation();
            for (const idSuperArc a : visit(saddleVert)) {
               Propagation* lowerProp = getArc(a).getPropagation();
               if (firstProp != lowerProp) {
                  firstProp->merge(*lowerProp);
                  closeArc(a, saddleId);
                  std::cout << "close " << printArc(a) << std::endl;
               }
            }
         }

         // Tools
         // -----

         void print(const int verbosity) const;

         std::string printArc(const idSuperArc arcId) const;

         std::string printNode(const idNode nodeId) const;

         // Initialize functions
         // --------------------

         void alloc() override;

         void init() override;

      };

   }
}

