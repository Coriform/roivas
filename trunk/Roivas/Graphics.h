#pragma once

//#define GLEW_STATIC
//#include <GL/glew.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include <FTGL/ftgl.h>
#include <assimp/Importer.hpp>
#include <assimp/types.h>
#include <assimp/scene.h> 
#include <assimp/postprocess.h> 
#include "SOIL.h"
#include "System.h"
#include "Model.h"
#include "Light.h"
#include "Transform.h"

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

namespace Roivas
{
	struct MeshData
	{
		GLuint MeshID;			
		GLuint VertexBuffer;
		GLuint UVBuffer;
		GLuint NormalBuffer;
		GLuint ElementBuffer;
		std::vector<unsigned short> Indices;
	};

	struct Attrib
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		bool operator<(const Attrib that) const{
			return memcmp((void*)this, (void*)&that, sizeof(Attrib))>0;
		};
	};

	class Graphics : public System
	{
		public:
			Graphics(SDL_Window* _window, float screenwidth, float screenheight);
			~Graphics();
			void Initialize();
			void PreloadAssets();
			GLuint LoadTexture(std::string path);
			void LoadMesh(std::string path, GLuint& vertexbuffer, GLuint& uvbuffer, GLuint& normalbuffer, GLuint& elementbuffer, std::vector<unsigned short>& indices);

			void Update(float dt);
			void UpdateScreenDims(int x, int y, int w, int h);
			void UpdateCameraRotation(float x, float y);
			void MouseSelectEntity(float x, float y, bool pressed);
			void UpdateLightPos(float x, float y);

			void AddComponent(Model* m);
			void AddComponent(Light* m);
			void RemoveComponent(Model* m);
			void RemoveComponent(Light* m);

			void CameraPitch(float angle);
			void CameraYaw(float angle);
			void CameraRoll(float angle);		

		private:
			void Draw3D(float dt);
			void DrawPP(float dt);			
			void DrawEditor(float dt);
			void DrawWireframe(float dt);		
			void Draw2D(float dt);
			void SortModels(float dt);
			void UpdateCamera(float dt);
			void DrawDebugText(std::string path);

			GLint CreateShaderProgram(std::string _vertSource, std::string _fragSource);			
			void LoadFontmap(std::string path);
			GLint LoadShader(std::string shader_filename, GLenum shader_type);
			//void ProcessVertexData(float* vertices, GLuint& mesh, GLuint& buff, unsigned size);
			void ProcessVertexData(std::vector<glm::vec3> & in_vertices,std::vector<glm::vec2> & in_uvs,std::vector<glm::vec3> & in_normals,	std::vector<unsigned short> & out_indices,
		std::vector<glm::vec3> & out_vertices,std::vector<glm::vec2> & out_uvs,std::vector<glm::vec3> & out_normals	);
			bool getSimilarVertexIndex_fast( 
	Attrib & packed, 
	std::map<Attrib,unsigned short> & VertexToOutIndex,
	unsigned short & result
	);
			void SetupFonts();
			void InitializeCamera();

			GLuint rboDepthStencil;
			GLuint meshCube, meshQuad;
			GLuint buffCube, buffQuad;
			SDL_Window*		window;

			mat4 modelMat, viewMat, projMat, orthoMat;
			GLuint uniColor, uniModel, uniView, uniProj, uniOrtho;
			GLuint uniLightPos, uniLightCol, uniLightRad, uniEyePos;
			GLuint wireColor, wireModel, wireView, wireProj;
			GLuint TextureID, ShadowMapID;
			GLuint MatrixID;
			GLuint ViewMatrixID;
			GLuint ProjMatrixID;
			GLuint ModelMatrixID;
			GLuint DepthBiasID;
			GLuint lightInvDirID;
			GLuint depthMatrixID;
			

			SDL_Surface *HUD;			

			GLuint screen_fbo;
			GLuint screen_tex;

			GLuint shadow_fbo;
			GLuint shadow_tex;

			void BindForWriting();
			void BindForReading(GLenum TextureUnit);

			std::map<std::string,GLuint> TEXTURE_LIST;
			std::map<std::string,MeshData> MESH_LIST;
			std::vector<GLuint> DEBUG_TEXT;
			std::vector<GLuint> FONTMAPS;
			std::vector<GLuint> SHADER_PROGRAMS;
			std::vector<GLuint> VERTEX_SHADERS;
			std::vector<GLuint> FRAGMENT_SHADERS;

			std::vector<Model*> MODEL_LIST;
			std::vector<Light*> LIGHT_LIST;

			float screen_width, screen_height;
			GLint screen_width_i, screen_height_i;		

			SDL_Surface*	debug_surface;
			std::stringstream debug_string;

			FTGLPixmapFont* font;
			//FTGLPolygonFont* font;

			const aiScene* scene;

			Uint32 ticks;
			Uint32 fps;
			std::string framerate;
			float accum;

			GLuint	base;
			GLfloat	cnt1;
			GLfloat	cnt2;

			vec3 cam_pos;
			vec3 cam_look;
			vec3 cam_up;
			vec3 cam_right; 
			quat cam_rot;

			unsigned varray_size;

			float pitch;

			Entity* SelectedEntity;


		// Comparator functor for sorting the model list by depth
		struct ZSorting
		{
			ZSorting( const Graphics& g) : graphics(g) {}			
			const Graphics& graphics;
			bool operator() ( Model* a, Model* b ) 
			{ 
				vec3 posa = a->GetTransform()->Position;
				vec3 posb = b->GetTransform()->Position;

				return glm::distance( posa, graphics.cam_pos ) < glm::distance( posb, graphics.cam_pos );
			}
		};

		
	};
}