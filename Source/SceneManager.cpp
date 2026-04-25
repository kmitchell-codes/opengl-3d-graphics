///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}

	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method loads texture images into memory for the
 *  3D scene and binds them to available texture slots.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// load the scratched metal texture for the sewing machine body
	bReturn = CreateGLTexture(
		"Textures\\black_metal_dark.jpg",
		"machine_body");
	
	// load the brushed metal texture for the sewing machine parts
	bReturn = CreateGLTexture(
		"Textures\\brushed_metal.jpg",
		"machine_parts");

	// load the black metal texture for the case
	bReturn = CreateGLTexture(
		"Textures\\black_metal.jpg",
		"case");

	// load the scratched metal texture for wooden elements
	bReturn = CreateGLTexture(
		"Textures\\weathered_wood.jpg",
		"desk");

	bReturn = CreateGLTexture(
		"Textures\\antique_wood.jpg",
		"empty_spool");
	
	// load the tomato pincushion texture 
	bReturn = CreateGLTexture(
		"Textures\\tomato_pincushion.jpg",
		"pin_cushion");
	
	// load thread texture 
	bReturn = CreateGLTexture(
		"Textures\\thread.jpg",
		"thread");
	

	// load fabric textures
	
	bReturn = CreateGLTexture(
		"Textures\\fabric1.jpg",
		"sunflower");

	bReturn = CreateGLTexture(
		"Textures\\fabric2.jpg",
		"art_deco");

	bReturn = CreateGLTexture(
		"Textures\\fabric3.jpg",
		"blue_check");

	bReturn = CreateGLTexture(
		"Textures\\fabric4.jpg",
		"red_check");
	
	bReturn = CreateGLTexture(
		"Textures\\fabric5.jpg",
		"red_square");

	// bind the loaded textures to texture slots
	BindGLTextures();
}
/***********************************************************
 *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{   
	//===== Wood material =====
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.ambientStrength = 0.45f;
	woodMaterial.diffuseColor = glm::vec3(0.35f, 0.28f, 0.20f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 8.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	//===== Metal material =====
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	metalMaterial.ambientStrength = 0.25f;
	metalMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.specularColor = glm::vec3(0.65f, 0.65f, 0.65f);
	metalMaterial.shininess = 30.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	//===== Fabric material =====
	OBJECT_MATERIAL fabricMaterial;
	fabricMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.2f);
	fabricMaterial.ambientStrength = 0.35f;
	fabricMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.4f);
	fabricMaterial.specularColor = glm::vec3(0.08f, 0.08f, 0.08f);
	fabricMaterial.shininess = 8.0f;
	fabricMaterial.tag = "fabric";
	m_objectMaterials.push_back(fabricMaterial);

	//===== Tomato pincushion material =====
	OBJECT_MATERIAL tomatoMaterial;
	tomatoMaterial.ambientColor = glm::vec3(0.45f, 0.08f, 0.06f);
	tomatoMaterial.ambientStrength = 0.25f;
	tomatoMaterial.diffuseColor = glm::vec3(0.85f, 0.16f, 0.12f);
	tomatoMaterial.specularColor = glm::vec3(0.18f, 0.12f, 0.10f);
	tomatoMaterial.shininess = 12.0f;
	tomatoMaterial.tag = "tomato";
	m_objectMaterials.push_back(tomatoMaterial);

}
 /***********************************************************
  *  SetupSceneLights()
  *
  *  This method is called to add and configure the light
  *  sources for the 3D scene.
  ***********************************************************/
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	

	// Key light: main room light, above and slightly to the right/front
	m_pShaderManager->setVec3Value("lightSources[0].position", 5.0f, 8.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.18f, 0.18f, 0.18f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.18f);

	// Fill light: softer, slightly cooler, opposite side
	m_pShaderManager->setVec3Value("lightSources[1].position", -5.0f, 4.0f, 4.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.12f, 0.12f, 0.12f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.4f, 0.4f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.08f);

	// Back rim light: behind and a little high for separation
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 7.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.08f, 0.08f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 6.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.10f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadPrismMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	/***                        Desk: Plane		    				***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("desk");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	
	//Draw sewing machine
	DrawMachine();

	//Draw pincushion 
	DrawPincushion();

	//Draw thread spools
	//3 Spool grouping left side machine 
	DrawThreadSpool(
		glm::vec3(-7.0f, 0.83f, 1.0f), //farthest right
		glm::vec4(0.55f, 0.18f, 0.20f, 1.0f),//color
		glm::vec3(0.40f, 1.2f, 0.40f) //size
	);

	DrawThreadSpool(
		glm::vec3(-8.5f, 0.83f, 1.8f), //farthest left
		glm::vec4(0.88f, 0.84f, 0.76f, 1.0f),//color
		glm::vec3(0.30f, 0.8f, 0.30f) //size
	);

	DrawThreadSpool(
		glm::vec3(-7.53f, 0.83f, 1.8f), // middle 
		glm::vec4(0.62f, 0.70f, 0.58f, 1.0f),//color
		glm::vec3(0.30f, 0.8f, 0.30f) //size
	);

	//3 spool grouping to right of machine
	DrawThreadSpool(
		glm::vec3(6.0f, 0.83f, 2.6f),//right
		glm::vec4(0.60f, 0.48f, 0.35f, 1.0f),//color
		glm::vec3(0.45f, 1.2f, 0.45f) //size
	);

	DrawThreadSpool(
		glm::vec3(5.2f, 0.83f, 1.90f),//middle
		glm::vec4(0.50f, 0.42f, 0.36f, 1.0f),//color
		glm::vec3(0.35f, 1.5f, 0.35f) //size
	);

	DrawThreadSpool(
		glm::vec3(4.0f, 0.83f, 2.3f), // left 
		glm::vec4(0.58f, 0.65f, 0.72f, 1.0),//color
		glm::vec3(0.30f, 0.8f, 0.30f) //size
	);

	//Single spool far right table
	DrawThreadSpool(
		glm::vec3(9.0f, 0.2f, 3.3f), // far right 
		glm::vec4(0.72f, 0.62f, 0.30f, 1.0f),//color
		glm::vec3(0.55f, 1.3f, 0.55f) //size
	);

	//Draw fabric stacks
	DrawFabricStack();

	/****************************************************************/
}

/***********************************************************
 *  DrawMachine()
 *
 *  This method draws the sewing machine object using
 *  multiple basic 3D shapes.
 ***********************************************************/
void SceneManager::DrawMachine()
{
    // declare the variables for the transformations
    glm::vec3 scaleXYZ;
    float XrotationDegrees = 0.0f;
    float YrotationDegrees = 0.0f;
    float ZrotationDegrees = 0.0f;
    glm::vec3 positionXYZ;

    /******************************************************************/
    /***              Sewing Machine: Base Platform                 ***/
    /******************************************************************/
    scaleXYZ = glm::vec3(14.0f, 0.48f, 8.0f);
    positionXYZ = glm::vec3(0.0f, 0.5f, 0.0f);

    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
    SetShaderTexture("desk");
	SetShaderMaterial("wood");
	SetTextureUVScale(2.0f, 2.0f);
    m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***              Sewing Machine: extension table               ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(14.0f, 0.48f, 8.0f);
	positionXYZ = glm::vec3(-14.2f, 0.5f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("desk");
	SetShaderMaterial("wood");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***              Sewing Machine: Machine Base                 ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(10.5f, 0.5f, 4.5f);
	positionXYZ = glm::vec3(0.0f, 0.5f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("machine_body");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

    /******************************************************************/
    /***               Sewing Machine: Main Body                   ***/
    /******************************************************************/
    scaleXYZ = glm::vec3(1.8f, 5.0f, 1.8f);
	
	// rotate around Y to move seam to the back
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.5f, 0.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("machine_body");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawTaperedCylinderMesh();
	
	/******************************************************************/
    /***               Sewing Machine: Arm                          ***/
    /******************************************************************/
	scaleXYZ = glm::vec3(1.0f, 6.4f, 1.4f);
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(3.5f, 5.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("machine_body");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawTaperedCylinderMesh();
	
	/******************************************************************/
	/***               Sewing Machine: Head                         ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(0.5f, 3.5f, 1.75f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.2f, 4.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("machine_body");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();
	
    /******************************************************************/
    /***               Sewing Machine: Side Wheel                   ***/
    /******************************************************************/
	scaleXYZ = glm::vec3(1.75f, 1.75f, 1.3f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.2f, 5.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("machine_parts");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	/******************************************************************/
    /***               Sewing Machine: Wheel Hub                    ***/
    /******************************************************************/
	scaleXYZ = glm::vec3(0.5f, 0.5f, 1.0f);

	// matching the torus orientation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// same position as the wheel
	positionXYZ = glm::vec3(4.0f, 5.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); 
	SetShaderTexture("machine_body");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	/***               Sewing Machine: tension rod                  ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(0.1f, 1.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.2f, 6.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("machine_parts");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/
	/***               Sewing Machine: Needle housing               ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(0.1f, 2.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.2f, 1.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); 
	SetShaderTexture("machine_parts");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	/***               Sewing Machine: presser foot                 ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.75f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.2f, 1.0f, 0.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); 
	SetShaderTexture("machine_parts");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***               Sewing Machine: spool pin                    ***/
	/******************************************************************/
	scaleXYZ = glm::vec3(0.1f, 1.75f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.0f, 5.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); 
	SetShaderTexture("machine_parts");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  DrawPincushion()
 *
 *  This method draws the pin cushion object using
 *  multiple basic 3D shapes.
 ***********************************************************/
void SceneManager::DrawPincushion()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Base: Black case
	scaleXYZ = glm::vec3(4.0f, 1.5f, 2.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-8.3f, 1.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); 
	SetShaderTexture("case");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// red tomato body
	scaleXYZ = glm::vec3(1.1f, 1.05f, 1.1f);
	positionXYZ = glm::vec3(-8.9f, 3.25f, 0.7f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("pin_cushion");
	SetShaderMaterial("tomato");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();	
}
	
/***********************************************************
 *  DrawThreadSpools()
 *
 *  This method draws the thread spool objects using
 *  multiple basic 3D shapes.
 ***********************************************************/
void SceneManager::DrawThreadSpool(glm::vec3 spoolPos, glm::vec4 spoolColor, glm::vec3 spoolScale)
{
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;

	// thread spool body
	scaleXYZ = spoolScale;
	positionXYZ = spoolPos + glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(spoolColor.r, spoolColor.g, spoolColor.b, spoolColor.a);
	SetShaderTexture("thread"); // same texture for all spools
	SetShaderMaterial("fabric");

	SetTextureUVScale(1.0f, 1.5f);
	m_basicMeshes->DrawCylinderMesh();

	// top lip (scaled relative to body)
	scaleXYZ = glm::vec3(spoolScale.x + 0.1f, 0.08f, spoolScale.z + 0.1f);
	positionXYZ = spoolPos + glm::vec3(0.0f, spoolScale.y, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.80f, 0.65f, 0.40f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();

	// bottom lip
	scaleXYZ = glm::vec3(spoolScale.x + 0.1f, 0.08f, spoolScale.z + 0.1f);
	positionXYZ = spoolPos + glm::vec3(0.0f, -0.1f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.80f, 0.65f, 0.40f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  DrawFabricStack()
 *
 *  This method draws the fabric stack object using
 *  multiple basic 3D shapes.
 ***********************************************************/
void SceneManager::DrawFabricStack()
{
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;

	// bottom fabric
	scaleXYZ = glm::vec3(5.2f, 0.75f, 6.4f);
	positionXYZ = glm::vec3(8.9f, 1.0f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, -40.0f, -8.0f, positionXYZ);
	//SetShaderColor(0.38f, 0.28f, 0.18f, 1.0f);
	SetShaderColor(0.9f, 0.85f, 0.8f, 1.0f);
	SetShaderTexture("art_deco");
	SetTextureUVScale(0.5f, 0.5f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();

	// second fabric
	scaleXYZ = glm::vec3(4.2f, 0.40f, 5.4f);
	positionXYZ = glm::vec3(9.2f, 1.55f, -2.9);
	SetTransformations(scaleXYZ, 0.0f, -40.0f, -8.0f, positionXYZ);
	SetShaderColor(0.9f, 0.85f, 0.8f, 1.0f);
	SetShaderTexture("red_check");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();

	// third fabric
	scaleXYZ = glm::vec3(3.0f, 0.40f, 4.2f);
	positionXYZ = glm::vec3(9.2f, 2.0f, -2.9);
	SetTransformations(scaleXYZ, 0.0f, -40.0f, -8.0f, positionXYZ);
	SetShaderColor(0.9f, 0.85f, 0.8f, 1.0f);
	SetShaderTexture("blue_check");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();

	// top  fourth fabric
	scaleXYZ = glm::vec3(2.5f, 0.40f, 3.2f);
	positionXYZ = glm::vec3(9.2f, 2.44f, -2.9);
	SetTransformations(scaleXYZ, 0.0f, -40.0f, -8.5f, positionXYZ);
	SetShaderColor(0.9f, 0.85f, 0.8f, 1.0f);
	SetShaderTexture("sunflower");
	SetTextureUVScale(2.2f, 2.2f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();

	// under machine fabric
	scaleXYZ = glm::vec3(4.0f, 0.40f, 5.2f);
	positionXYZ = glm::vec3(-2.0, 0.7, 0.0);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.9f, 0.85f, 0.8f, 1.0f);
	SetShaderTexture("red_square");
	SetTextureUVScale(2.0f, 2.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();
}

