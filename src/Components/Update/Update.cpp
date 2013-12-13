/*!
 * \file
 * \brief
 * \author Micha Laszkowski
 */

#include <memory>
#include <string>

#include "Update.hpp"
#include "Common/Logger.hpp"

#include <boost/bind.hpp>
///////////////////////////////////////////
#include <string>
#include <cmath>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/point_representation.h>

#include "pcl/impl/instantiate.hpp"
#include "pcl/search/kdtree.h" 
#include "pcl/search/impl/kdtree.hpp"
#include <pcl/registration/correspondence_estimation.h>
#include "pcl/registration/correspondence_rejection_sample_consensus.h"

#include <pcl/io/pcd_io.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/visualization/point_cloud_color_handlers.h>
////////////////////////////////////////////////////////////////////////
namespace Processors {
namespace Update {

class SIFTFeatureRepresentation: public pcl::DefaultFeatureRepresentation <PointXYZSIFT> //could possibly be pcl::PointRepresentation<...> ??
{
	using pcl::PointRepresentation<PointXYZSIFT>::nr_dimensions_;
	public:
	SIFTFeatureRepresentation ()
	{
		// Define the number of dimensions
		nr_dimensions_ = 128 ;
		trivial_ = false ;
	}

	// Override the copyToFloatArray method to define our feature vector
	virtual void copyToFloatArray (const PointXYZSIFT &p, float * out) const
	{
		//This representation is only for determining correspondences (not for use in Kd-tree for example - so use only SIFT part of the point	
		for (register int i = 0; i < 128 ; i++)
			out[i] = p.descriptor[i];//p.descriptor.at<float>(0, i) ;
		//std::cout << "SIFTFeatureRepresentation:copyToFloatArray()" << std::endl ;
	}
};



Eigen::Matrix4f computeTransformationSAC(const pcl::PointCloud<PointXYZSIFT>::ConstPtr &cloud_src, const pcl::PointCloud<PointXYZSIFT>::ConstPtr &cloud_trg, 
		const pcl::CorrespondencesConstPtr& correspondences, pcl::Correspondences& inliers)
{
	std::cout << "Computing SAC" << std::endl ;	
	std::cout.flush() ;
	pcl::registration::CorrespondenceRejectorSampleConsensus<PointXYZSIFT> sac ;
	sac.setInputSource(cloud_src) ;
	sac.setInputTarget(cloud_trg) ;
	sac.setInlierThreshold(0.001f) ;
	sac.setMaximumIterations(2000) ;
	sac.setInputCorrespondences(correspondences) ;
	sac.getCorrespondences(inliers) ;
	std::cout << "SAC inliers " << inliers.size() << std::endl ;
	return sac.getBestTransformation() ;
}


///////////////////////////////////////////////////////////////////
Update::Update(const std::string & name) :
		Base::Component(name)  {

}

Update::~Update() {
}

void Update::prepareInterface() {
	// Register data streams, events and event handlers HERE!
registerStream("in_cloud_xyzrgb", &in_cloud_xyzrgb);
registerStream("in_cloud_xyzsift", &in_cloud_xyzsift);
registerStream("out_instance", &out_instance);
registerStream("out_cloud", &out_cloud);
	// Register handlers
	h_update.setup(boost::bind(&Update::update, this));
	registerHandler("update", &h_update);
	addDependency("update", &in_cloud_xyzsift);
	addDependency("update", &in_cloud_xyzrgb);

}

bool Update::onInit() {
	counter = 0;
	global_trans = Eigen::Matrix4f::Identity();


	cloud_prev = pcl::PointCloud<pcl::PointXYZRGB>::Ptr (new pcl::PointCloud<pcl::PointXYZRGB>());
	cloud_next = pcl::PointCloud<pcl::PointXYZRGB>::Ptr (new pcl::PointCloud<pcl::PointXYZRGB>());
	cloud_to_merge = pcl::PointCloud<pcl::PointXYZRGB>::Ptr (new pcl::PointCloud<pcl::PointXYZRGB>());
	cloud_sift_prev = pcl::PointCloud<PointXYZSIFT>::Ptr (new pcl::PointCloud<PointXYZSIFT>());
	cloud_sift_next = pcl::PointCloud<PointXYZSIFT>::Ptr (new pcl::PointCloud<PointXYZSIFT>());	
	
	return true;
}

bool Update::onFinish() {
	return true;
}

bool Update::onStop() {
	return true;
}

bool Update::onStart() {
	return true;
}

void Update::update() {
	cout<<"Update()"<<endl;
	
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud = in_cloud_xyzrgb.read();
	pcl::PointCloud<PointXYZSIFT>::Ptr cloud_sift = in_cloud_xyzsift.read();
	
	//first clouds
	if (counter == 0 ){
		cloud_merged = cloud;
		cloud_prev = cloud;
		cloud_sift_merged = cloud_sift;
		cloud_sift_prev = cloud_sift;
		counter++;
		out_cloud.write(cloud_merged);
		return;
	}
	
	cloud_next = cloud;
	cloud_sift_next = cloud_sift;
	counter++;
	
	
	
		pcl::CorrespondencesPtr correspondences(new pcl::Correspondences()) ;
		pcl::registration::CorrespondenceEstimation<PointXYZSIFT, PointXYZSIFT> correst ;
	
		//SIFTFeatureRepresentation point_representation ;
		//correst.setPointRepresentation (point_representation.makeShared()); //NEVER do like this, makeShared will return DefaultFeatureRepresentation<PointDefault>!
		SIFTFeatureRepresentation::Ptr point_representation(new SIFTFeatureRepresentation()) ;
		correst.setPointRepresentation(point_representation) ;
		correst.setInputSource(cloud_sift_next) ;
		correst.setInputTarget(cloud_sift_prev) ;
		correst.determineReciprocalCorrespondences(*correspondences) ;
		std::cout << "\nNumber of reciprocal correspondences: " << correspondences->size() << " out of " << cloud_sift_next->size() << " keypoints" << std::endl ;
	
		//for (pcl::PointCloud<PointXYZRGBSIFT>::iterator pt_iter = cloud_sift_next->begin(); pt_iter != cloud_sift_next->end() ; pt_iter++) {
		//	displayMatrixInfo(pt_iter->descriptor) ;			
		//	std::cout << "Matrix value " << pt_iter->descriptor << std::endl ;
		//}
		
		//displayCorrespondences(cloud_next, cloud_sift_next, cloud_prev, cloud_sift_prev, correspondences, viewer) ;

		//Compute transformation between clouds and update global transformation of cloud_next	
		pcl::Correspondences inliers ;
		Eigen::Matrix4f current_trans = computeTransformationSAC(cloud_sift_next, cloud_sift_prev, correspondences, inliers) ;
		std::cout << "Transformation cloud_next -> cloud_prev: " << std::endl << current_trans << std::endl ;
		global_trans = global_trans * current_trans ;

		//Merge cloud - cloud_next
		pcl::transformPointCloud(*cloud_next, *cloud_to_merge, global_trans) ;
		//addCloudToScene(cloud_to_merge, sceneviewer, counter - 1) ; 
		*cloud_merged = *cloud_merged + *cloud_to_merge ;

		*cloud_prev = *cloud_next ;
		*cloud_sift_prev = *cloud_sift_next ;


	
	
		out_cloud.write(cloud_merged);
	cout<< "Counter: " << counter <<endl;
}



} //: namespace Update
} //: namespace Processors