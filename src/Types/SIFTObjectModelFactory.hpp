#ifndef SIFTOBJECTMODELFACTORY_HPP_
#define SIFTOBJECTMODELFACTORY_HPP_

#include <Types/AbstractObjectFactory.hpp> 
#include <Types/SIFTObjectModel.hpp> 


class SIFTObjectModelFactory : public AbstractObjectFactory
{
public:
	SIFTObjectModelFactory(){}
	~SIFTObjectModelFactory(){}
    AbstractObject* produce(){ /* ziamplementowana - na podstawie tych dwóch chmur (pól) i nazwy zwraca obiekt*/
		SIFTObjectModel *som = new SIFTObjectModel;
		som->cloud = cloud_xyzrgb;
		som->SIFTcloud = cloud_xyzsift;
		som->name = model_name;
		return som;
}
	//add_to_vector(SOM);
	
protected:
	pcl::PointCloud<PointXYZSIFT>::Ptr cloud_xyzsift;// (new pcl::PointCloud<PointXYZSIFT>());
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_xyzrgb;// (new pcl::PointCloud<pcl::PointXYZ>());
	string model_name;
	//vector<SOM>
	
};
#endif /* SIFTOBJECTMODELFACTORY_HPP_ */
