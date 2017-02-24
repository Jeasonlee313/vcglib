/****************************************************************************
* VCGLib                                                            o o     *
* Visual and Computer Graphics Library                            o     o   *
*                                                                _   O  _   *
* Copyright(C) 2004-2016                                           \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
#ifndef __VCGLIB_POLY_MESH_ALGORITHM
#define __VCGLIB_POLY_MESH_ALGORITHM

#include <vcg/complex/algorithms/update/normal.h>
#include <vcg/complex/complex.h>
#include <vcg/space/polygon3.h>
#include <vcg/complex/algorithms/update/color.h>
#include <vcg/complex/algorithms/closest.h>
#include <vcg/complex/algorithms/update/flag.h>
#include <vcg/complex/algorithms/point_sampling.h>

//define a temporary triangle mesh type
class TempFace;
class TempVertex;

struct TempUsedTypes: public vcg::UsedTypes<vcg::Use<TempVertex>::AsVertexType,
        vcg::Use<TempFace>::AsFaceType>{};

class TempVertex:public vcg::Vertex<TempUsedTypes,
        vcg::vertex::Coord3d,
        vcg::vertex::Normal3d,
        vcg::vertex::BitFlags>
{};

class TempFace:public vcg::Face<TempUsedTypes,
        vcg::face::VertexRef,
        vcg::face::BitFlags,
        vcg::face::FFAdj,
        vcg::face::Mark,
        vcg::face::Normal3d>
{};


class TempMesh: public vcg::tri::TriMesh< std::vector<TempVertex>,std::vector<TempFace > >
{};

namespace vcg{

/*!
\ingroup PolyMeshType

\headerfile color.h vcg/complex/algorithms/polygonal_algorithms.h

\brief processing and optimization of generic polygonal meshes.

This class is used to performs varisous kind of geometric optimization on generic polygonal mesh such as flattengin or imptove the shape of polygons.
*/

template <class PolyMeshType>
class PolygonalAlgorithm
{
    typedef typename PolyMeshType::FaceType FaceType;
    typedef typename PolyMeshType::VertexType VertexType;
    typedef typename PolyMeshType::CoordType CoordType;
    typedef typename PolyMeshType::ScalarType ScalarType;
    typedef typename vcg::face::Pos<FaceType> PosType;
public:
    static bool CollapseEdges(PolyMeshType &poly_m,
                             const std::vector<PosType> &CollapsePos,
                             const std::vector<CoordType> &InterpPos)
    {

        //this set how to remap the vertices after deletion
        std::map<VertexType*,VertexType*> VertexRemap;

        vcg::tri::UpdateFlags<PolyMeshType>::VertexClearS(poly_m);

        bool collapsed=false;
        //go over all faces and check the ones needed to be deleted
        for (size_t i=0;i<CollapsePos.size();i++)
        {
            FaceType *currF=CollapsePos[i].F();
            int IndexE=CollapsePos[i].E();
            size_t NumV=currF->VN();
            VertexType *v0=currF->V(IndexE);
            VertexType *v1=currF->V((IndexE+1)%NumV);

            //safety check
            assert(v0!=v1);

            if (v0->IsS())continue;
            if (v1->IsS())continue;

            //put on the same position
            v0->P()=InterpPos[i];
            v1->P()=InterpPos[i];

            //select the the two vertices
            v0->SetS();
            v1->SetS();

            //set the remap
            VertexRemap[v1]=v0;

            collapsed=true;
        }

        //then remap vertices
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            int NumV=poly_m.face[i].VN();
            for (int j=0;j<NumV;j++)
            {
                //get the two vertices of the edge
                VertexType *v0=poly_m.face[i].V(j);
                //see if it must substituted or not
                if (VertexRemap.count(v0)==0)continue;
                //in that case remap to the new one
                VertexType *newV=VertexRemap[v0];
                //assign new vertex
                poly_m.face[i].V(j)=newV;
            }
        }

        //then re-elaborate the face
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            //get vertices of the face
            int NumV=poly_m.face[i].VN();
            std::vector<VertexType*> FaceV;
            for (int j=0;j<NumV;j++)
            {
                VertexType *v0=poly_m.face[i].V(j);
                VertexType *v1=poly_m.face[i].V((j+1)%NumV);
                if(v0==v1)continue;
                FaceV.push_back(v0);
            }

            //then deallocate face
            if ((int)FaceV.size()==NumV)continue;

            //otherwise deallocate and set new vertices
            poly_m.face[i].Dealloc();
            poly_m.face[i].Alloc(FaceV.size());
            for (size_t j=0;j<FaceV.size();j++)
                poly_m.face[i].V(j)=FaceV[j];
        }

        //remove unreferenced vertices
        vcg::tri::Clean<PolyMeshType>::RemoveUnreferencedVertex(poly_m);

        //and compact them
        vcg::tri::Allocator<PolyMeshType>::CompactEveryVector(poly_m);

        return collapsed;
    }
private:
//    static bool CollapseBorderSmallEdgesStep(PolyMeshType &poly_m,
//                                             const ScalarType edge_limit)
//    {
//        bool collapsed=false;

//        //update topology
//        vcg::tri::UpdateTopology<PolyMeshType>::FaceFace(poly_m);

//        //update border vertices
//        //UpdateBorderVertexFromPFFAdj(poly_m);
//        vcg::tri::UpdateFlags<PolyMeshType>::VertexBorderFromFaceAdj(poly_m);

//        //get border edges
//        std::vector<std::vector<bool> > IsBorder;
//        //BorderEdgeFromPFFAdj(poly_m,IsBorder);
//        vcg::tri::UpdateFlags<PolyMeshType>::VertexBorderFromFaceAdj(poly_m);
//        //deselect all vertices
//        vcg::tri::UpdateFlags<PolyMeshType>::VertexClearS(poly_m);

//        //this set how to remap the vertices after deletion
//        std::map<VertexType*,VertexType*> VertexRemap;

//        //go over all faces and check the ones needed to be deleted
//        for (size_t i=0;i<poly_m.face.size();i++)
//        {
//            int NumV=poly_m.face[i].VN();
//            for (size_t j=0;j<NumV;j++)
//            {
//                VertexType *v0=poly_m.face[i].V(j);
//                VertexType *v1=poly_m.face[i].V((j+1)%NumV);
//                assert(v0!=v1);

//                bool IsBV0=v0->IsB();
//                bool IsBV1=v1->IsB();

//                //in these cases is not possible to collapse
//                if ((!IsBV0)&&(!IsBV1))continue;
//                //if ((!IsBorder[i][j])&&(IsBV0)&&(IsBV1))continue;
//                bool IsBorderE=poly_m.face[i].FFp(j);
//                if ((IsBorderE)&&(IsBV0)&&(IsBV1))continue;
//                if (v0->IsS())continue;
//                if (v1->IsS())continue;

//                assert((IsBV0)||(IsBV1));
//                CoordType pos0=v0->P();
//                CoordType pos1=v1->P();
//                ScalarType currL=(pos0-pos1).Norm();
//                if (currL>edge_limit)continue;

//                //then collapse the point
//                CoordType InterpPos;
//                if ((IsBV0)&&(!IsBV1))InterpPos=pos0;
//                if ((!IsBV0)&&(IsBV1))InterpPos=pos1;
//                if ((IsBV0)&&(IsBV1))InterpPos=(pos0+pos1)/2.0;

//                //put on the same position
//                v0->P()=InterpPos;
//                v1->P()=InterpPos;

//                //select the the two vertices
//                v0->SetS();
//                v1->SetS();

//                //set the remap
//                VertexRemap[v1]=v0;

//                collapsed=true;
//            }
//        }

//        //then remap vertices
//        for (size_t i=0;i<poly_m.face.size();i++)
//        {
//            int NumV=poly_m.face[i].VN();
//            for (int j=0;j<NumV;j++)
//            {
//                //get the two vertices of the edge
//                VertexType *v0=poly_m.face[i].V(j);
//                //see if it must substituted or not
//                if (VertexRemap.count(v0)==0)continue;
//                //in that case remap to the new one
//                VertexType *newV=VertexRemap[v0];
//                //assign new vertex
//                poly_m.face[i].V(j)=newV;
//            }
//        }

//        //then re-elaborate the face
//        for (size_t i=0;i<poly_m.face.size();i++)
//        {
//            //get vertices of the face
//            int NumV=poly_m.face[i].VN();
//            std::vector<VertexType*> FaceV;
//            for (int j=0;j<NumV;j++)
//            {
//                VertexType *v0=poly_m.face[i].V(j);
//                VertexType *v1=poly_m.face[i].V((j+1)%NumV);
//                if(v0==v1)continue;
//                FaceV.push_back(v0);
//            }

//            //then deallocate face
//            if ((int)FaceV.size()==NumV)continue;

//            //otherwise deallocate and set new vertices
//            poly_m.face[i].Dealloc();
//            poly_m.face[i].Alloc(FaceV.size());
//            for (size_t j=0;j<FaceV.size();j++)
//                poly_m.face[i].V(j)=FaceV[j];
//        }

//        return collapsed;
//    }

    static bool CollapseBorderSmallEdgesStep(PolyMeshType &poly_m,
                                             const ScalarType edge_limit)
    {
        bool collapsed=false;

        //update topology
        vcg::tri::UpdateTopology<PolyMeshType>::FaceFace(poly_m);

        //update border vertices
        vcg::tri::UpdateFlags<PolyMeshType>::VertexBorderFromFaceAdj(poly_m);


        std::vector<PosType> CollapsePos;
        std::vector<CoordType> InterpPos;

        //go over all faces and check the ones needed to be deleted
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            int NumV=poly_m.face[i].VN();
            for (size_t j=0;j<NumV;j++)
            {
                VertexType *v0=poly_m.face[i].V(j);
                VertexType *v1=poly_m.face[i].V((j+1)%NumV);
                assert(v0!=v1);

                bool IsBV0=v0->IsB();
                bool IsBV1=v1->IsB();

                //in these cases is not possible to collapse
                if ((!IsBV0)&&(!IsBV1))continue;
                bool IsBorderE=poly_m.face[i].FFp(j);
                if ((IsBorderE)&&(IsBV0)&&(IsBV1))continue;

                assert((IsBV0)||(IsBV1));
                CoordType pos0=v0->P();
                CoordType pos1=v1->P();
                ScalarType currL=(pos0-pos1).Norm();
                if (currL>edge_limit)continue;

                //then collapse the point
                CoordType CurrInterpPos;
                if ((IsBV0)&&(!IsBV1))CurrInterpPos=pos0;
                if ((!IsBV0)&&(IsBV1))CurrInterpPos=pos1;
                if ((IsBV0)&&(IsBV1))CurrInterpPos=(pos0+pos1)/2.0;

                CollapsePos.push_back(PosType(&poly_m.face[i],j));
                InterpPos.push_back(CurrInterpPos);
            }
        }

        return CollapseEdges(poly_m,CollapsePos,InterpPos);
    }

    static void LaplacianPos(PolyMeshType &poly_m,std::vector<CoordType> &AvVert)
    {
        //cumulate step
        AvVert.clear();
        AvVert.resize(poly_m.vert.size(),CoordType(0,0,0));
        std::vector<ScalarType> AvSum(poly_m.vert.size(),0);
        for (size_t i=0;i<poly_m.face.size();i++)
            for (size_t j=0;j<(size_t)poly_m.face[i].VN();j++)
            {
                //get current vertex
                VertexType *currV=poly_m.face[i].V(j);
                //and its position
                CoordType currP=currV->P();
                //cumulate over other positions
                ScalarType W=vcg::PolyArea(poly_m.face[i]);
                //assert(W!=0);
                for (size_t k=0;k<(size_t)poly_m.face[i].VN();k++)
                {
                    if (k==j)continue;
                    int IndexV=vcg::tri::Index(poly_m,poly_m.face[i].V(k));
                    AvVert[IndexV]+=currP*W;
                    AvSum[IndexV]+=W;
                }
            }

        //average step
        for (size_t i=0;i<poly_m.vert.size();i++)
        {
            if (AvSum[i]==0)continue;
            AvVert[i]/=AvSum[i];
        }
    }



    static void UpdateNormal(FaceType &F)
    {
        F.N()=vcg::PolygonNormal(F);
    }

    static void UpdateNormalByFitting(FaceType &F)
    {
        UpdateNormal(F);
        vcg::Plane3<ScalarType> PlF;
        PlF=PolyFittingPlane(F);
        if ((PlF.Direction()*F.N())<0)
            F.N()=-PlF.Direction();
        else
            F.N()=PlF.Direction();
    }

public:

    
    static CoordType GetFaceGetBary(FaceType &F)
    {
        CoordType bary=PolyBarycenter(F);
        return bary;
    }

    /*! \brief update the face normal by averaging among vertex's
     * normals computed between adjacent edges
    */
    static void UpdateFaceNormals(PolyMeshType &poly_m)
    {
        for (size_t i=0;i<poly_m.face.size();i++)
            UpdateNormal(poly_m.face[i]);
    }

    /*! \brief update the face normal by fitting a plane
    */
    static void UpdateFaceNormalByFitting(PolyMeshType &poly_m)
    {
        for (size_t i=0;i<poly_m.face.size();i++)
            UpdateNormalByFitting(poly_m.face[i]);
    }

    //    ///METTERE SOTTO TOPOLOGY?
    //    static void  BorderEdgeFromPFFAdj(FaceType &F,std::vector<bool> &IsBorder)
    //    {
    //        IsBorder.resize(F.VN(),false);
    //        for (int j=0;j<F.VN();j++)
    //            IsBorder[j]=(F.FFp(j)==&F);
    //    }

    //    static void BorderEdgeFromPFFAdj(PolyMeshType &poly_m,
    //                                     std::vector<std::vector<bool> > &IsBorder)
    //    {
    //        IsBorder.resize(poly_m.face.size());
    //        for (size_t i=0;i<poly_m.face.size();i++)
    //        {
    //            if (poly_m.face[i].IsD())continue;
    //            BorderEdgeFromPFFAdj(poly_m.face[i],IsBorder[i]);
    //        }
    //    }

    //    static void UpdateBorderVertexFromPFFAdj(PolyMeshType &poly_m)
    //    {
    //        //get per edge border flag
    //        std::vector<std::vector<bool> > IsBorderE;
    //        BorderEdgeFromPFFAdj(poly_m,IsBorderE);

    //        //then update per vertex
    //        vcg::tri::UpdateFlags<PolyMeshType>::VertexClearB(poly_m);
    //        for (size_t i=0;i<poly_m.face.size();i++)
    //        {
    //            if (poly_m.face[i].IsD())continue;

    //            for (int j=0;j<poly_m.face[i].VN();j++)
    //            {
    //                if (!IsBorderE[i][j])continue;
    //                poly_m.face[i].V0(j)->SetB();
    //                poly_m.face[i].V1(j)->SetB();
    //            }
    //        }
    //    }
    

    enum PolyQualityType{QAngle,QPlanar,QTemplate};

    /*! \brief update the quality of the faces by considering different possibilities
     * QAngle   = consider the angle deviation from ideal one (ex 90° quad, 60° triangle...)
     * QPlanar  = consider the difference wrt interpolating plane
     * QTemplate= consider the difference wrt template polygon as in "Statics Aware Grid Shells"
    */
    static void UpdateQuality(PolyMeshType &poly_m,
                              const PolyQualityType &QType)
    {
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            if (poly_m.face[i].IsD())continue;
            switch (QType)
            {
            case QAngle:
                ScalarType AvgDev,WorstDev;
                vcg::PolyAngleDeviation(poly_m.face[i],AvgDev,WorstDev);
                poly_m.face[i].Q()=AvgDev;
                break;
            case QPlanar:
                poly_m.face[i].Q()=vcg::PolyFlatness(poly_m.face[i]);
                break;
            default:
                poly_m.face[i].Q()=vcg::PolyAspectRatio(poly_m.face[i],true);
                break;
            }
        }
    }

    /*! \brief given a face this function returns the template positions as in "Statics Aware Grid Shells"
    */
    static void GetRotatedTemplatePos(FaceType &f,
                                      std::vector<CoordType> &TemplatePos)
    {
        vcg::GetPolyTemplatePos(f,TemplatePos,true);

        CoordType NormT=Normal(TemplatePos);
        //get the normal of vertices
        CoordType AVN(0,0,0);
        CoordType Origin(0,0,0);
        for (int j=0;j<f.VN();j++)
            AVN+=f.V(j)->N();

        for (size_t j=0;j<TemplatePos.size();j++)
            Origin+=TemplatePos[j];

        Origin/=(ScalarType)TemplatePos.size();
        AVN.Normalize();

        //find rotation matrix
        vcg::Matrix33<ScalarType> Rot=vcg::RotationMatrix(NormT,AVN);

        //apply transformation
        for (size_t j=0;j<TemplatePos.size();j++)
        {
            TemplatePos[j]-=Origin;
            TemplatePos[j]=Rot*TemplatePos[j];
            TemplatePos[j]+=Origin;
        }
    }

    /*! \brief This function performs the polygon regularization as in "Statics Aware Grid Shells"
    */
    static void SmoothPCA(PolyMeshType &poly_m,
                          int relax_step=10,
                          ScalarType Damp=0.5,
                          bool fixIrr=false,
                          bool isotropic=true,
                          ScalarType smoothTerm=0.1,
                          bool fixB=true)
    {
        (void)isotropic;
        typedef typename PolyMeshType::FaceType PolygonType;
        //        // select irregular ones
        //        if (fixIrr)
        //            poly_m.NumIrregular(true);
        // compute the average edge
        ScalarType MeshArea=0;
        for (size_t i=0;i<poly_m.face.size();i++)
            MeshArea+=vcg::PolyArea(poly_m.face[i]);

        ScalarType AvgArea=MeshArea/(ScalarType)poly_m.face.size();

        for (size_t s=0;s<(size_t)relax_step;s++)
        {
            //initialize the accumulation vector
            std::vector<CoordType> avgPos(poly_m.vert.size(),CoordType(0,0,0));
            std::vector<ScalarType> weightSum(poly_m.vert.size(),0);
            //then compute the templated positions
            for (size_t i=0;i<poly_m.face.size();i++)
            {
                std::vector<typename PolygonType::CoordType> TemplatePos;
                GetRotatedTemplatePos(poly_m.face[i],TemplatePos);
                //then cumulate the position per vertex
                ScalarType val=vcg::PolyArea(poly_m.face[i]);
                if (val<(AvgArea*0.00001))
                    val=(AvgArea*0.00001);
                ScalarType W=1.0/val;//poly_m.face[i].Q();
                //ScalarType W=1;

                for (size_t j=0;j<TemplatePos.size();j++)
                {
                    int IndexV=vcg::tri::Index(poly_m,poly_m.face[i].V(j));
                    CoordType Pos=TemplatePos[j];
                    //sum up contributes
                    avgPos[IndexV]+=Pos*W;
                    weightSum[IndexV]+=W;
                }
            }

            //get the laplacian contribute
            std::vector<CoordType> AvVert;
            LaplacianPos(poly_m,AvVert);
            //then update the position
            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                ScalarType alpha=smoothTerm;//PolyNormDeviation(poly_m.face[i]);
                //               if (alpha<0)alpha=0;
                //               if (alpha>1)alpha=1;
                //               if (isnan(alpha))alpha=1;
                CoordType newP=avgPos[i]/weightSum[i];
                newP=newP*(1-alpha)+AvVert[i]*alpha;
                if ((fixB)&&(poly_m.vert[i].IsB()))continue;
                if ((fixIrr)&&(poly_m.vert[i].IsS()))continue;
                poly_m.vert[i].P()=poly_m.vert[i].P()*Damp+
                        newP*(1-Damp);
            }
        }
    }

    /*! \brief This function smooth the borders of the polygonal mesh and reproject back to the triangolar one
     * except the vertices that are considered as corner wrt the angleDeg threshold
    */
    template <class TriMeshType>
    static void LaplacianReprojectBorder(PolyMeshType &poly_m,
                                         TriMeshType &tri_mesh,
                                         int nstep=100,
                                         ScalarType Damp=0.5,
                                         ScalarType angleDeg=100)
    {
        typedef typename TriMeshType::FaceType FaceType;

        //first select corners
        vcg::tri::UpdateFlags<PolyMeshType>::VertexClearS(poly_m);

        //update topology
        vcg::tri::UpdateTopology<PolyMeshType>::FaceFace(poly_m);

        //update border vertices
        vcg::tri::UpdateFlags<PolyMeshType>::VertexBorderFromFaceAdj(poly_m);

        //select corner vertices on the border
        ScalarType angleRad=angleDeg * M_PI / 180;
        vcg::tri::UpdateFlags<PolyMeshType>::SelectVertexCornerBorder(poly_m,angleRad);

        for (int s=0;s<nstep;s++)
        {
            std::vector<CoordType> AvVert;
            LaplacianPos(poly_m,AvVert);

            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                if (!poly_m.vert[i].IsB())continue;
                if (poly_m.vert[i].IsS())continue;
                poly_m.vert[i].P()=poly_m.vert[i].P()*Damp+
                        AvVert[i]*(1-Damp);
            }

            //then reproject on border
            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                if (!poly_m.vert[i].IsB())continue;
                if (poly_m.vert[i].IsS())continue;

                CoordType testPos=poly_m.vert[i].P();
                ScalarType minD=std::numeric_limits<ScalarType>::max();
                CoordType closPos;
                for (size_t j=0;j<tri_mesh.face.size();j++)
                    for (size_t k=0;k<3;k++)
                    {
                        if (!tri_mesh.face[j].IsB(k))continue;

                        CoordType P0,P1;
                        P0.Import(tri_mesh.face[j].cP0(k));
                        P1.Import(tri_mesh.face[j].cP1(k));
                        vcg::Segment3<ScalarType> Seg(P0,P1);
                        ScalarType testD;
                        CoordType closTest;
                        vcg::SegmentPointDistance(Seg,testPos,closTest,testD);
                        if (testD>minD)continue;
                        minD=testD;
                        closPos=closTest;
                    }
                poly_m.vert[i].P()=closPos;
            }
        }
    }

    /*! \brief This function smooth the borders of the polygonal mesh and reproject back to its border
    */
    static void LaplacianReprojectBorder(PolyMeshType &poly_m,
                                         int nstep=100,
                                         ScalarType Damp=0.5)
    {
        //transform into triangular
        TempMesh GuideSurf;
        vcg::tri::PolygonSupport<TempMesh,PolyMeshType>::ImportFromPolyMesh(GuideSurf,poly_m);
        vcg::tri::UpdateBounding<TempMesh>::Box(GuideSurf);
        vcg::tri::UpdateNormal<TempMesh>::PerVertexNormalizedPerFace(GuideSurf);
        vcg::tri::UpdateTopology<TempMesh>::FaceFace(GuideSurf);
        vcg::tri::UpdateFlags<TempMesh>::FaceBorderFromFF(GuideSurf);

        LaplacianReprojectBorder<TempMesh>(poly_m,GuideSurf,nstep,Damp);
    }

    /*! \brief This function performs the reprojection of the polygonal mesh onto a triangular one passed as input parameter
    */
    template <class TriMeshType>
    static void LaplacianReproject(PolyMeshType &poly_m,
                                   TriMeshType &tri_mesh,
                                   bool fixIrr=false,
                                   int nstep=100,
                                   ScalarType Damp=0.5)
    {
        typedef typename TriMeshType::FaceType FaceType;
        typedef vcg::GridStaticPtr<FaceType, ScalarType> TriMeshGrid;
        TriMeshGrid grid;

        //initialize the grid
        grid.Set(tri_mesh.face.begin(),tri_mesh.face.end());

        ScalarType MaxD=tri_mesh.bbox.Diag();

        if (fixIrr)
            poly_m.NumIrregular(true);
        for (int s=0;s<nstep;s++)
        {
            std::vector<CoordType> AvVert;
            LaplacianPos(poly_m,AvVert);

            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                if (poly_m.vert[i].IsB())continue;
                if (fixIrr && (poly_m.vert[i].IsS()))continue;
                poly_m.vert[i].P()=poly_m.vert[i].P()*Damp+
                        AvVert[i]*(1-Damp);
            }


            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                CoordType testPos=poly_m.vert[i].P();
                CoordType closestPt;
                ScalarType minDist;
                FaceType *f=NULL;
                CoordType norm,ip;
                f=vcg::tri::GetClosestFaceBase(tri_mesh,grid,testPos,MaxD,minDist,closestPt,norm,ip);
                poly_m.vert[i].P()=poly_m.vert[i].P()*Damp+
                        closestPt*(1-Damp);
                poly_m.vert[i].N()=norm;
            }
        }

    }

    /*! \brief This function performs the polygon regularization as in "Statics Aware Grid Shells"
     * followed by a reprojection step on the triangle mesh passed as parameter
    */
    template <class TriMeshType>
    static void SmoothReprojectPCA(PolyMeshType &poly_m,
                                   TriMeshType &tri_mesh,
                                   int relaxStep=100,
                                   bool fixIrr=false,
                                   ScalarType Damp=0.5)
    {
        vcg::tri::UpdateTopology<PolyMeshType>::FaceFace(poly_m);

        //UpdateBorderVertexFromPFFAdj(poly_m);
        vcg::tri::UpdateFlags<PolyMeshType>::VertexBorderFromFaceAdj(poly_m);

        typedef typename TriMeshType::FaceType FaceType;
        typedef vcg::GridStaticPtr<FaceType, typename TriMeshType::ScalarType> TriMeshGrid;
        TriMeshGrid grid;

        //initialize the grid
        grid.Set(tri_mesh.face.begin(),tri_mesh.face.end());

        ScalarType MaxD=tri_mesh.bbox.Diag();

        //        //update quality as area
        //        for (size_t i=0;i<poly_m.face.size();i++)
        //          poly_m.face[i].Q()=vcg::PolyArea(poly_m.face[i]);

        for (size_t i=0;i<poly_m.vert.size();i++)
        {
            typename TriMeshType::CoordType testPos;
            testPos.Import(poly_m.vert[i].P());
            typename TriMeshType::CoordType closestPt;
            typename TriMeshType::ScalarType minDist;
            typename TriMeshType::FaceType *f=NULL;
            typename TriMeshType::CoordType norm,ip;
            f=vcg::tri::GetClosestFaceBase(tri_mesh,grid,testPos,MaxD,minDist,closestPt,norm,ip);
            poly_m.vert[i].N().Import(norm);
        }

        for(int k=0;k<relaxStep;k++)
        {
            //smooth PCA step
            SmoothPCA(poly_m,1,Damp,fixIrr);
            //reprojection step
            //laplacian smooth step
            //Laplacian(poly_m,Damp,1);

            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                typename TriMeshType::CoordType testPos;
                testPos.Import(poly_m.vert[i].P());
                typename TriMeshType::CoordType closestPt;
                typename TriMeshType::ScalarType minDist;
                FaceType *f=NULL;
                typename TriMeshType::CoordType norm,ip;
                f=vcg::tri::GetClosestFaceBase(tri_mesh,grid,testPos,MaxD,minDist,closestPt,norm,ip);
                poly_m.vert[i].P().Import(testPos*Damp+closestPt*(1-Damp));
                poly_m.vert[i].N().Import(norm);
            }
        }
    }

    /*! \brief This function performs the polygon regularization as in "Statics Aware Grid Shells"
     * followed by a reprojection step on the original mesh
    */
    static void SmoothReprojectPCA(PolyMeshType &poly_m,
                                   int relaxStep=100,
                                   bool fixIrr=false,
                                   ScalarType Damp=0.5)
    {
        //transform into triangular
        TempMesh GuideSurf;
        vcg::tri::PolygonSupport<TempMesh,PolyMeshType>::ImportFromPolyMesh(GuideSurf,poly_m);
        vcg::tri::UpdateBounding<TempMesh>::Box(GuideSurf);
        vcg::tri::UpdateNormal<TempMesh>::PerVertexNormalizedPerFace(GuideSurf);
        vcg::tri::UpdateTopology<TempMesh>::FaceFace(GuideSurf);
        vcg::tri::UpdateFlags<TempMesh>::FaceBorderFromFF(GuideSurf);

        //optimize it
        vcg::PolygonalAlgorithm<PolyMeshType>::SmoothReprojectPCA<TempMesh>(poly_m,GuideSurf,relaxStep,fixIrr,Damp);
    }

    /*! \brief This function return average edge size
    */
    static ScalarType AverageEdge(PolyMeshType &poly_m)
    {
        ScalarType AvL=0;
        size_t numE=0;
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            int NumV=poly_m.face[i].VN();
            for (size_t j=0;j<NumV;j++)
            {
                CoordType pos0=poly_m.face[i].V(j)->P();
                CoordType pos1=poly_m.face[i].V((j+1)%NumV)->P();
                AvL+=(pos0-pos1).Norm();
                numE++;
            }
        }
        AvL/=numE;
        return AvL;
    }
    

    /*! \brief This function remove valence 2 faces from the mesh
    */
    static void RemoveValence2Faces(PolyMeshType &poly_m)
    {
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            if (poly_m.face[i].VN()>=3)continue;
            vcg::tri::Allocator<PolyMeshType>::DeleteFace(poly_m,poly_m.face[i]);
        }

        //then remove unreferenced vertices
        vcg::tri::Clean<PolyMeshType>::RemoveUnreferencedVertex(poly_m);
        vcg::tri::Allocator<PolyMeshType>::CompactEveryVector(poly_m);

    }
    
    /*! \brief This function remove valence 2 vertices on the border by considering the degree threshold
     * bacause there could be eventually some corner that should be preserved
    */
    static void RemoveValence2BorderVertices(PolyMeshType &poly_m,
                                             ScalarType corner_degree=25)
    {
        //update topology
        vcg::tri::UpdateTopology<PolyMeshType>::FaceFace(poly_m);

        //update border vertices
        //UpdateBorderVertexFromPFFAdj(poly_m);
        vcg::tri::UpdateFlags<PolyMeshType>::VertexBorderFromFaceAdj(poly_m);

        vcg::tri::UpdateFlags<PolyMeshType>::VertexClearS(poly_m);

        //select corners
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            if (poly_m.face[i].IsD())continue;

            //get vertices of the face
            int NumV=poly_m.face[i].VN();

            for (size_t j=0;j<NumV;j++)
            {
                VertexType *v0=poly_m.face[i].V((j+NumV-1)%NumV);
                VertexType *v1=poly_m.face[i].V(j);
                VertexType *v2=poly_m.face[i].V((j+1)%NumV);
                //must be 3 borders
                if ((!v0->IsB())||(!v1->IsB())||(!v2->IsB()))continue;
                CoordType dir0=(v0->P()-v1->P());
                CoordType dir1=(v2->P()-v1->P());
                dir0.Normalize();
                dir1.Normalize();
                ScalarType testDot=(dir0*dir1);
                if (testDot>(-cos(corner_degree* (M_PI / 180.0))))
                    v1->SetS();
            }
        }

        typename PolyMeshType::template PerVertexAttributeHandle<size_t> valenceVertH =
                vcg::tri::Allocator<PolyMeshType>:: template GetPerVertexAttribute<size_t> (poly_m);

        //initialize to zero
        for (size_t i=0;i<poly_m.vert.size();i++)
            valenceVertH[i]=0;

        //then sum up the valence
        for (size_t i=0;i<poly_m.face.size();i++)
            for (int j=0;j<poly_m.face[i].VN();j++)
                valenceVertH[poly_m.face[i].V(j)]++;

        //then re-elaborate the faces
        for (size_t i=0;i<poly_m.face.size();i++)
        {
            if (poly_m.face[i].IsD())continue;

            //get vertices of the face
            int NumV=poly_m.face[i].VN();

            std::vector<VertexType*> FaceV;
            for (size_t j=0;j<NumV;j++)
            {
                VertexType *v=poly_m.face[i].V(j);
                assert(!v->IsD());
                if ((!v->IsS()) && (v->IsB()) && (valenceVertH[v]==1)) continue;
                FaceV.push_back(v);
            }

            //then deallocate face
            if (FaceV.size()==NumV)continue;

            //otherwise deallocate and set new vertices
            poly_m.face[i].Dealloc();
            poly_m.face[i].Alloc(FaceV.size());
            for (size_t j=0;j<FaceV.size();j++)
                poly_m.face[i].V(j)=FaceV[j];
        }

        //then remove unreferenced vertices
        vcg::tri::Clean<PolyMeshType>::RemoveUnreferencedVertex(poly_m);
        vcg::tri::Allocator<PolyMeshType>::CompactEveryVector(poly_m);

        vcg::tri::Allocator<PolyMeshType>::DeletePerVertexAttribute(poly_m,valenceVertH);
    }

    /*! \brief This function collapse small edges which are on the boundary of the mesh
     * this is sometimes useful to remove small edges coming out from a quadrangulation which is not
     * aligned to boundaries
    */
    static void CollapseBorderSmallEdges(PolyMeshType &poly_m,const ScalarType perc_average=0.3)
    {
        //compute the average edge
        ScalarType AvEdge=AverageEdge(poly_m);
        ScalarType minLimit=AvEdge*perc_average;

        while(CollapseBorderSmallEdgesStep(poly_m,minLimit)){};

        RemoveValence2Faces(poly_m);

        RemoveValence2BorderVertices(poly_m);
    }

    /*! \brief This function use a local global approach to flatten polygonal faces
     * the approach is similar to "Shape-Up: Shaping Discrete Geometry with Projections"
    */
    static ScalarType FlattenFaces(PolyMeshType &poly_m, size_t steps=100,bool OnlySFaces=false)
    {
        ScalarType MaxDispl=0;
        for (size_t s=0;s<steps;s++)
        {
            std::vector<std::vector<CoordType> > VertPos(poly_m.vert.size());

            for (size_t i=0;i<poly_m.face.size();i++)
            {
                if (poly_m.face[i].IsD())continue;

                if (OnlySFaces && (!poly_m.face[i].IsS()))continue;
                //get vertices of the face
                int NumV=poly_m.face[i].VN();
                if (NumV<=3)continue;

                //save vertice's positions
                std::vector<CoordType> FacePos;
                for (int j=0;j<NumV;j++)
                {
                    VertexType *v=poly_m.face[i].V(j);
                    assert(!v->IsD());
                    FacePos.push_back(v->P());
                }

                //then fit the plane
                vcg::Plane3<ScalarType> FitPl;
                vcg::FitPlaneToPointSet(FacePos,FitPl);

                //project each point onto fitting plane
                for (int j=0;j<NumV;j++)
                {
                    VertexType *v=poly_m.face[i].V(j);
                    int IndexV=vcg::tri::Index(poly_m,v);
                    CoordType ProjP=FitPl.Projection(v->P());
                    VertPos[IndexV].push_back(ProjP);
                }
            }

            for (size_t i=0;i<poly_m.vert.size();i++)
            {
                CoordType AvgPos(0,0,0);

                for (size_t j=0;j<VertPos[i].size();j++)
                    AvgPos+=VertPos[i][j];

                if (VertPos[i].size()==0)continue;

                AvgPos/=(ScalarType)VertPos[i].size();

                MaxDispl=std::max(MaxDispl,(poly_m.vert[i].P()-AvgPos).Norm());
                poly_m.vert[i].P()=AvgPos;
            }
        }
        return MaxDispl;
    }

};

}//end namespace vcg

#endif
