#include "render.h"
#include "stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "direct.h"
#include "utils.h"
using namespace std;
using namespace cv;
using namespace pugi;


// Load xml file
void ParseXml(std::string xml_path, pugi::xml_document& doc){
	doc.load_file(xml_path.data());
}

// Dump xml object to file
void DumpXml(const pugi::xml_document& doc, std::string file_path){
	doc.save_file(file_path.data());
}

// Modify emitter in xml file
void ModifyXmlLight(pugi::xml_document& doc, const cv::Vec3d& light_direction){
	xml_node node = doc.child("scene").child("emitter").child("vector");
	node.attribute("x").set_value(light_direction[0]);
	node.attribute("y").set_value(light_direction[1]);
	node.attribute("z").set_value(light_direction[2]);
}

// Run mitsuba to render the scene
void RenderScene(std::string mitsuba_path, std::string xml_path, std::string output_path){
	string command = mitsuba_path + "\\mitsuba.exe " + xml_path + " -q -o " + output_path;
	system(command.data());

	// also store the rendering result in png
	command = mitsuba_path + "\\mtsutil.exe tonemap " + output_path + ".exr";
	system(command.data());
}

// Map cubemap coordinates to 3D coordinates
// Note: mitsuba's coordinate system is quite special, with x to the left, y to to the top, and z to the inside
cv::Vec3d CubeLightDirection(double u, double v, int face_index){
	double a = 1;
	double b = 2 * u - 1;
	double c = 2 * v - 1;
	Vec3d direction(0, 0, 0);
	switch (face_index){
	case 0: direction = Vec3d(-a, -c, -b); break;
	case 1: direction = Vec3d(a, -c, b); break;
	case 2: direction = Vec3d(-b, a, c); break;
	case 3: direction = Vec3d(-b, -a, -c); break;
	case 4: direction = Vec3d(-b, -c, a); break;
	case 5: direction = Vec3d(b, -c, -a); break;
	default:exit(-1);
	}
	direction = normalize(direction);
	return -1.0 * direction;
}


void RenderCubemap(string mitsuba_path, std::string scene_path, std::string target_dir, int cubemap_length){
	xml_document doc;
	ParseXml(scene_path, doc);

	PrepareDirectory(target_dir);

	int index = scene_path.find_last_of("/\\");
	string scene_dir = scene_path.substr(0, index);
	string temp_scene_path = scene_dir + "\\" + "temp_scene.xml";
	string env_path = scene_dir + "\\" + "temp_env.exr";
	for (int i = 0; i < 6; i++){
		string output_dir = target_dir + "\\face_" + to_string(i);
		int count = 0;
		for (int j = 0; j < cubemap_length; j++){
			for (int k = 0; k < cubemap_length; k++){
				printf("Rendering face %d: (%d, %d)...\n", i, j, k);
				
				// create cubemap
				vector<Mat> cubemap;
				CreateCubemap(i, k, j, 2, cubemap);
				Mat envmap;
				ConvertCubemapToLightProbe(envmap, cubemap, Size(256, 128));
				imwrite(env_path, envmap);

				// Vec3d direction;
				// double v = (j + 0.5) / (double)cubemap_length;
				// double u = (k + 0.5) / (double)cubemap_length;
				// direction = CubeLightDirection(u, v, i);
				// ModifyXmlLight(doc, direction);
				// DumpXml(doc, temp_scene_path);
				
				string output_path = output_dir + "\\" + to_string(count);
				RenderScene(mitsuba_path, temp_scene_path, output_path);
				count++;
			}
		}
	}
}

// Prepare directory to store results.
void PrepareDirectory(std::string target_dir){
	struct stat info;
	int status = stat(target_dir.data(), &info);
	if (status == 0 && info.st_mode && S_IFDIR){
		// directory exists, do nothing
	}
	else{
		// create directory
		if (_mkdir(target_dir.c_str()) != 0){
			printf("Fail to create directroy %s\n", target_dir);
			exit(-1);
		}
	}

	// create 6 directories to store data for each cubemap face
	for (int i = 0; i < 6; i++){
		string dir_name = target_dir + "\\face_" + to_string(i);
		_mkdir(dir_name.c_str());
	}
}

void StartRendering(){
	string mitsuba_path = "C:\\Users\\bisai\\Downloads\\Mitsuba";
	string scene_path = "C:\\Users\\bisai\\Documents\\research\\prt\\scenes\\matpreview.xml";
	string target_dir = "C:\\Users\\bisai\\Documents\\research\\prt\\data1";
	int cubemap_length = 2;

	RenderCubemap(mitsuba_path, scene_path, target_dir, cubemap_length);
}