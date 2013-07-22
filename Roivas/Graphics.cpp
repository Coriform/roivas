#include "CommonLibs.h"
#include "Graphics.h"
#include "Body.h"		// DELETE
#include "Entity.h"		// DELETE
#include "FileIO.h"		// DELETE
#include "Level.h"		// DELETE



//// TUT
#include "objloader.hpp"
#include "vboindexer.hpp"
////

namespace Roivas
{
	Graphics::Graphics(SDL_Window* _window, float s_width, float s_height) : System(SYS_Graphics, "Graphics"), 
		window(_window), 
		screen_width(s_width),
		screen_height(s_height),
		screen_width_i((GLint)s_width),
		screen_height_i((GLint)s_height),
		debug_surface(nullptr),
		ticks(0),
		fps(0),
		pitch(0.0f),
		accum(0.0f),
		varray_size(8),
		SelectedEntity(nullptr)
	{
		// Initialize ticks counted for framerate
		ticks = SDL_GetTicks();

		// Vsync = 1; Immediate = 0
		SDL_GL_SetSwapInterval(0);

	}

	void Graphics::Initialize()
	{
		// Reset matrix
		glLoadIdentity();

		// Clear color
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

		// Shader model - Use this
		glShadeModel(GL_SMOOTH);
 
		//2D rendering
		glMatrixMode(GL_PROJECTION);
	
	//// TUT
 //
		// Enable depth checking
		glEnable(GL_DEPTH_TEST);

		////
		glDepthFunc(GL_LESS);	// TUT
		////

		// Enable alpha blending
		glEnable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glEnable(GL_ALPHA_TEST);
		//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		////
		glEnable(GL_CULL_FACE);	// TUT
		////
////

		// Create and assign fonts
		SetupFonts();

		// Initialize Camera
		InitializeCamera();
		




		// Build Shaders - automate this somehow
		// Make sure this is in same order as enum list
		CreateShaderProgram("Assets/Shaders/Default.vert",	"Assets/Shaders/Default.frag");
		CreateShaderProgram("Assets/Shaders/Phong.vert",	"Assets/Shaders/Phong.frag");
		CreateShaderProgram("Assets/Shaders/ShadowTex.vert","Assets/Shaders/ShadowTex.frag");
		CreateShaderProgram("Assets/Shaders/Screen.vert",	"Assets/Shaders/Screen.frag");
		CreateShaderProgram("Assets/Shaders/Hud.vert",		"Assets/Shaders/Hud.frag");
		CreateShaderProgram("Assets/Shaders/Wireframe.vert","Assets/Shaders/Wireframe.frag");

		// Preload meshes and textures
		PreloadAssets();

		//// TUT Preload!
		TutPreload();

		// Reset default framebuffer
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		Level* lvl = new Level("TestLevel.json");
		lvl->Load();
	}

	void Graphics::Update(float dt)
	{
		// Calculate framerate, based on an increment every 1000 milliseconds
		if( SDL_GetTicks() - ticks >= 1000 )
		{
			std::stringstream ss;
			ss << "FPS: " << fps;
			framerate = ss.str();
			std::cout << "Frame rate frames per second: " << framerate << std::endl;
			//std::cout << "Light pos: " << light->GetTransform()->Position.x << "," << light->GetTransform()->Position.y << "," << light->GetTransform()->Position.z << "," << std::endl;
			fps = 0;
			ticks = SDL_GetTicks();
		}

		// Actual framerate value
		++fps;

		//// commented for TUT
		//SortModels(dt);

		// Updates current camera
		UpdateCamera(dt);

		// Primary drawing functions; draws the geometry and lighting calculations
		//Draw3D(dt);		


		//// TUT
		DrawTut(dt);
		////


		// Post-processing effects
		//DrawPP(dt);

		// Debug drawing
		//DrawEditor(dt);
		//DrawWireframe(dt);

		// HUD and other 2D drawing
		Draw2D(dt);	

		// Draw framerate
		DrawDebugText(framerate);	

		// Clears debug text from previous frame; might change this
		DEBUG_TEXT.clear();
	
		// Update window with OpenGL context; Swap buffers
		SDL_GL_SwapWindow(window);
	}




	//// TUT
	void Graphics::TutPreload()
	{
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		depthProgramID = CreateShaderProgram( "Assets/Shaders/DepthRTT.vertexshader", "Assets/Shaders/DepthRTT.fragmentshader" );

		// Get a handle for our "MVP" uniform
		depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

		// Read our .obj file		
		loadOBJ("room_thickwalls.obj", vertices, uvs, normals);

		
		indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);


		// Load it into a VBO
		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &uvbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

		glGenBuffers(1, &normalbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

		// Generate a buffer for the indices as well
		glGenBuffers(1, &elementbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

		// ---------------------------------------------
		// Render to Texture - specific code begins here
		// ---------------------------------------------


		// Create and compile our GLSL program from the shaders
		programID = CreateShaderProgram( "Assets/Shaders/ShadowMapping.vertexshader", "Assets/Shaders/ShadowMapping.fragmentshader" );

		// Get a handle for our "myTextureSampler" uniform
		TextureID  = glGetUniformLocation(programID, "myTextureSampler");

		// Get a handle for our "MVP" uniform
		MatrixID = glGetUniformLocation(programID, "MVP");
		ViewMatrixID = glGetUniformLocation(programID, "V");
		ModelMatrixID = glGetUniformLocation(programID, "M");
		DepthBiasID = glGetUniformLocation(programID, "DepthBiasMVP");
		ShadowMapID = glGetUniformLocation(programID, "shadowMap");
	
		// Get a handle for our "LightPosition" uniform
		lightInvDirID = glGetUniformLocation(programID, "LightInvDirection_worldspace");

	}
	////



	void Graphics::PreloadAssets()
	{

		meshCube = LoadMesh("Box.obj");;
		MESH_LIST["Box"] = meshCube;
		MESH_VERTICES[meshCube] = 36;

		// Quad vertices
		float quadVertices[] = {
			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f,  1.0f,  1.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			 1.0f, -1.0f,  1.0f, 0.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			-1.0f,  1.0f,  0.0f, 1.0f
		};

		glGenVertexArrays( 1, &meshQuad );
		glGenBuffers( 1, &buffQuad );

		glBindBuffer( GL_ARRAY_BUFFER, buffQuad );
		glBufferData( GL_ARRAY_BUFFER, sizeof( quadVertices ), quadVertices, GL_DYNAMIC_DRAW );

		glBindVertexArray( meshQuad );

			GLuint posAttrib = glGetAttribLocation( SHADER_PROGRAMS.at(SH_Screen), "position" );
			glEnableVertexAttribArray( posAttrib );
			glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), 0 );

			GLuint texAttrib = glGetAttribLocation( SHADER_PROGRAMS.at(SH_Screen), "texcoord" );
			glEnableVertexAttribArray( texAttrib );
			glVertexAttribPointer( texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)( 2 * sizeof( float ) ) );
			
		MESH_LIST["Quad"] = meshQuad;
		MESH_VERTICES[meshQuad] = 6;

		////
		
		glGenFramebuffers( 1, &screen_fbo );
		glBindFramebuffer( GL_FRAMEBUFFER, screen_fbo );

		glGenTextures( 1, &screen_tex );
		glBindTexture( GL_TEXTURE_2D, screen_tex );

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, screen_width_i, screen_height_i, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_tex, 0 );

		glGenRenderbuffers( 1, &rboDepthStencil );
		glBindRenderbuffer( GL_RENDERBUFFER, rboDepthStencil );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screen_width_i, screen_height_i );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil );

		////



		// Shadow map
		////

		// Create the FBO
		glGenFramebuffers(1, &shadow_fbo);    
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);


		glGenTextures(1, &shadow_tex);
		glBindTexture(GL_TEXTURE_2D, shadow_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, screen_width_i, screen_height_i, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_tex, 0);
		//glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_tex, 0 );

		//glDrawBuffer(GL_NONE); 

		glDrawBuffer(GL_NONE);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if( status != GL_FRAMEBUFFER_COMPLETE ) 
		{
			std::cout << "FB error, status: " << status << std::endl;
			return;
		} 

		////


		uniView		= glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "view" );
		uniProj		= glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "proj" );
		uniModel	= glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "model" );
		uniColor	= glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "overrideColor" );
		uniLightPos = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "lightpos" );
		uniLightCol = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "lightcolor" );
		uniLightRad = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "lightradius" );
		uniEyePos	= glGetUniformLocation( SHADER_PROGRAMS.at(SH_Phong), "eyepos" );

		wireView = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Wireframe), "view" );
		wireProj = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Wireframe), "proj" );
		wireModel = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Wireframe), "model" );
		wireColor = glGetUniformLocation( SHADER_PROGRAMS.at(SH_Wireframe), "wirecolor" );

		modelMat = mat4();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Graphics::BindForWriting()
	{
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, shadow_fbo);
	} 

	void Graphics::BindForReading(GLenum TextureUnit)
	{
		glActiveTexture(TextureUnit);
		glBindTexture(GL_TEXTURE_2D, shadow_tex);
	} 

	void Graphics::DrawTut(float dt)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

		// Render to our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
		glViewport(0,0,screen_width_i,screen_height_i); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		// We don't use bias in the shader, but instead we draw back faces, 
		// which are already separated from the front faces by a small distance 
		// (if your geometry is made this way)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(depthProgramID);

		glm::vec3 lightInvDir = glm::vec3(0.5f,2,2);

		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10,10,-10,10,-10,20);
		glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0,0,0), glm::vec3(0,1,0));

		glm::mat4 depthModelMatrix = glm::mat4(1.0);
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);



		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,screen_width_i,screen_height_i); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		glm::mat4 ProjectionMatrix = projMat;
		glm::mat4 ViewMatrix = viewMat;
		//ViewMatrix = glm::lookAt(glm::vec3(14,6,4), glm::vec3(0,1,0), glm::vec3(0,1,0));
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		
		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0, 
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
		);

		glm::mat4 depthBiasMVP = biasMatrix*depthMVP;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(DepthBiasID, 1, GL_FALSE, &depthBiasMVP[0][0]);

		glUniform3f(lightInvDirID, lightInvDir.x, lightInvDir.y, lightInvDir.z);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, MODEL_LIST.at(1)->DiffuseID);
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadow_tex);
		glUniform1i(ShadowMapID, 1);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);





		// Optionally render the shadowmap (for debug only)

		// Render only on a corner of the window (or we we won't see the real rendering...)
		glViewport(screen_width_i/2+screen_width_i/4,screen_height_i/2+screen_height_i/4,screen_width_i/4,screen_height_i/4);

		// Use our shader
		glUseProgram(SHADER_PROGRAMS.at(SH_Screen));
		glBindVertexArray( meshQuad );

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadow_tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(meshQuad) );

	}

	void Graphics::Draw3D(float dt)
	{
		glPolygonMode(GL_FRONT, GL_FILL);

		accum += dt;

		


		float light_positions[6];
		float light_colors[6];
		float light_radius[2];

		for( unsigned i = 0; i < LIGHT_LIST.size(); ++i )
		{
			vec3 pos = LIGHT_LIST[i]->GetTransform()->Position;
			light_positions[i*3]	= pos.x;
			light_positions[i*3+1]	= pos.y;
			light_positions[i*3+2]	= pos.z;

			vec3 color = LIGHT_LIST[i]->Color;
			light_colors[i*3]		= color.x;
			light_colors[i*3+1]		= color.y;
			light_colors[i*3+2]		= color.z;

			light_radius[i]			= LIGHT_LIST[i]->Radius;
		}



	


		// Shadows

		glBindFramebuffer( GL_FRAMEBUFFER, shadow_fbo );

		glViewport(0,0,screen_width_i,screen_height_i);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glClear( GL_DEPTH_BUFFER_BIT );
		

		glUseProgram( SHADER_PROGRAMS.at(SH_ShadowTex) );


		glm::vec3 lightInvDir = glm::vec3(0.5f,2,2);
		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10,10,-10,10,-10,20);
		glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0,0,0), glm::vec3(0,1,0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0);
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		vec3 light_pos (2.0f, 10.0f, 0.0f);
		vec3 light_target (2.0f, 0.0f, 0.0f);
		vec3 up_dir (0.0f, 0.0f, -1.0f);

		depthViewMatrix = glm::lookAt( light_pos, light_target, up_dir );

		float nearf = 1.0f;
		float farf = 100.0f;
		float fov = 45.0f;
		float aspect = 1.0f;
		mat4 caster_proj_mat = glm::perspective (fov, aspect, nearf, farf);
		

		for( unsigned i = 0; i < MODEL_LIST.size(); ++i )
		{
			glBindVertexArray( MODEL_LIST[i]->MeshID );		// Use the cube mesh		
		
			glUniformMatrix4fv( uniView, 1, GL_FALSE, MatToArray( depthViewMatrix ) );
			glUniformMatrix4fv( uniProj, 1, GL_FALSE, MatToArray( depthProjectionMatrix ) );

			Transform* t = MODEL_LIST[i]->GetTransform();

			modelMat = mat4();			

			modelMat = glm::translate( modelMat, t->Position );

			if( MODEL_LIST[i]->Owner->GetBehavior() != nullptr )
				modelMat = glm::rotate( modelMat, (accum/2000.0f) * 180.0f, vec3( 0.0f, 1.0f, 0.0f ) );	

			modelMat = glm::rotate( modelMat, t->Rotation.x, vec3( 1.0f, 0.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.y, vec3( 0.0f, 1.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.z, vec3( 0.0f, 0.0f, 1.0f ) );

			modelMat = glm::scale( modelMat, t->Scale );

			glUniform3f( uniColor, 1.0f, 1.0f, 1.0f );

			glUniform3fv( uniLightPos, 2, light_positions);
			glUniform3fv( uniLightCol, 2, light_colors);
			glUniform1fv( uniLightRad, 2, light_radius);

			glUniform3f( uniEyePos, cam_pos.x, cam_pos.y, cam_pos.z );
			glUniformMatrix4fv( uniModel, 1, GL_FALSE, MatToArray( modelMat ) );		// Pass the locally transformed model matrix to the scene shader	

			glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(MODEL_LIST[i]->MeshID) );	// Draw first cube


		}





		glBindFramebuffer( GL_FRAMEBUFFER, screen_fbo );	// RW; uses frameBuffer object to store and read from
		// If I put this ABOVE glBindFrameBuffer, acts like it doesn't clear
		// Maybe: I bind the buffer, then I clear THE BUFFER???

		glDisable( GL_CULL_FACE );

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		


		for( unsigned i = 0; i < MODEL_LIST.size(); ++i )
		{
			glEnable( GL_DEPTH_TEST );			// Enable z buffering / occlusion testing

			glBindVertexArray( MODEL_LIST[i]->MeshID );		// Use the cube mesh		
		
			glUseProgram( SHADER_PROGRAMS.at(SH_Phong) );		// Activate phong shader



			glUniformMatrix4fv( uniView, 1, GL_FALSE, MatToArray( viewMat ) );
			glUniformMatrix4fv( uniProj, 1, GL_FALSE, MatToArray( projMat ) );


			glActiveTexture( GL_TEXTURE0 );					
			glBindTexture( GL_TEXTURE_2D, MODEL_LIST[i]->DiffuseID );		// Stores TEXTURES.at(0) into texture unit 0

			Transform* t = MODEL_LIST[i]->GetTransform();

			modelMat = mat4();			

			modelMat = glm::translate( modelMat, t->Position );

			if( MODEL_LIST[i]->Owner->GetBehavior() != nullptr )
				modelMat = glm::rotate( modelMat, (accum/2000.0f) * 180.0f, vec3( 0.0f, 1.0f, 0.0f ) );	

			modelMat = glm::rotate( modelMat, t->Rotation.x, vec3( 1.0f, 0.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.y, vec3( 0.0f, 1.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.z, vec3( 0.0f, 0.0f, 1.0f ) );

			modelMat = glm::scale( modelMat, t->Scale );

			glUniform3f( uniColor, 1.0f, 1.0f, 1.0f );

			glUniform3fv( uniLightPos, 2, light_positions);
			glUniform3fv( uniLightCol, 2, light_colors);
			glUniform1fv( uniLightRad, 2, light_radius);

			glUniform3f( uniEyePos, cam_pos.x, cam_pos.y, cam_pos.z );
			glUniformMatrix4fv( uniModel, 1, GL_FALSE, MatToArray( modelMat ) );		// Pass the locally transformed model matrix to the scene shader	

			glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(MODEL_LIST[i]->MeshID) );	// Draw first cube


		}


		// Bind default framebuffer and draw contents of our framebuffer
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );		// Default system frame buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glDisable( GL_DEPTH_TEST );

		glBindVertexArray( meshQuad );
		
		glUseProgram( SHADER_PROGRAMS.at(SH_Screen) );

		glActiveTexture( GL_TEXTURE0 );

		glBindTexture( GL_TEXTURE_2D, screen_tex );

		// Screen quad; This contains the entire scene
		glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(meshQuad) );





		////

		glBindVertexArray( meshQuad );
		glViewport(0,0,400,400);
		glUseProgram( SHADER_PROGRAMS.at(SH_Screen) );

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, shadow_tex );

		//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
		//glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );

		glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(meshQuad) );

	}

	void Graphics::DrawPP(float dt)
	{
		// GUI drawing

	}

	void Graphics::Draw2D(float dt)
	{
		// GUI drawing


		
	}

	void Graphics::DrawWireframe(float dt)
	{		
		glViewport(0,0,screen_width_i,screen_height_i);

		// Draw wireframe geometry
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);

		for( unsigned i = 0; i < MODEL_LIST.size(); ++i )
		{
			glBindVertexArray( MODEL_LIST[i]->MeshID );		// Use the cube mesh		
		
			glUseProgram( SHADER_PROGRAMS.at(SH_Wireframe) );		// Activate phong shader

			glUniformMatrix4fv( wireView, 1, GL_FALSE, MatToArray( viewMat ) );
			glUniformMatrix4fv( wireProj, 1, GL_FALSE, MatToArray( projMat ) );

			Transform* t = MODEL_LIST[i]->GetTransform();
			vec3 color = MODEL_LIST[i]->WireColor;

			modelMat = mat4();		

			modelMat = glm::translate( modelMat, t->Position );		

			if( MODEL_LIST[i]->Owner->GetBehavior() != nullptr )
				modelMat = glm::rotate( modelMat, (accum/2000.0f) * 180.0f, vec3( 0.0f, 1.0f, 0.0f ) );

			modelMat = glm::rotate( modelMat, t->Rotation.x, vec3( 1.0f, 0.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.y, vec3( 0.0f, 1.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.z, vec3( 0.0f, 0.0f, 1.0f ) );

			modelMat = glm::scale( modelMat, t->Scale );						

			glUniform3f( wireColor, color.x, color.y, color.z );
			glUniformMatrix4fv( wireModel, 1, GL_FALSE, MatToArray( modelMat ) );		// Pass the locally transformed model matrix to the scene shader		
			glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(MODEL_LIST[i]->MeshID) );	// Draw first cube
		}

		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);	
	}

	void Graphics::DrawEditor(float dt)
	{		
		glViewport(0,0,screen_width_i,screen_height_i);

		// Draw wireframe geometry
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);

		for( unsigned i = 0; i < MODEL_LIST.size(); ++i )
		{
			if( MODEL_LIST[i]->MeshName != "Light.dae" )
				continue;

			glBindVertexArray( MODEL_LIST[i]->MeshID );		// Use the cube mesh		
		
			glUseProgram( SHADER_PROGRAMS.at(SH_Wireframe) );		// Activate phong shader

			glUniformMatrix4fv( wireView, 1, GL_FALSE, MatToArray( viewMat ) );
			glUniformMatrix4fv( wireProj, 1, GL_FALSE, MatToArray( projMat ) );

			Transform* t = MODEL_LIST[i]->GetTransform();
			vec3 color = MODEL_LIST[i]->WireColor;

			modelMat = mat4();		
			modelMat = glm::translate( modelMat, t->Position );	
			modelMat = glm::rotate( modelMat, t->Rotation.x, vec3( 1.0f, 0.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.y, vec3( 0.0f, 1.0f, 0.0f ) );
			modelMat = glm::rotate( modelMat, t->Rotation.z, vec3( 0.0f, 0.0f, 1.0f ) );
			modelMat = glm::scale( modelMat, t->Scale );
			

			glUniform3f( wireColor, color.x, color.y, color.z );
			glUniformMatrix4fv( wireModel, 1, GL_FALSE, MatToArray( modelMat ) );		// Pass the locally transformed model matrix to the scene shader		
			glDrawArrays( GL_TRIANGLES, 0, MESH_VERTICES.at(MODEL_LIST[i]->MeshID) );	// Draw first cube
		}

		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);	
	}

	void Graphics::DrawDebugText(std::string text)
	{
		if( font->Error() )
			return;

		float offset = font->LineHeight() / screen_height;		// Add this stuff to the function signature

		glUseProgram(0);		// Set current shader to null
		glColor3f(1.0, 1.0, 1.0); 
		//glRasterPos2f(-0.99f, 1-offset*2);
		font->Render(text.c_str());	
	}

	GLuint Graphics::LoadTexture(std::string path)
	{
		// Need to add code to assign texture GLuint to model

		if( TEXTURE_LIST.find(path) != TEXTURE_LIST.end() )
		{
			return TEXTURE_LIST.at(path);
		}

		int width, height;
		unsigned char* image = nullptr;

		std::string fullpath = "Assets/Textures/" + path;

		image = SOIL_load_image( fullpath.c_str(), &width, &height, 0, SOIL_LOAD_RGB );

		if( image == nullptr )
		{
			std::cout << "Texture didn't load properly" << std::endl;
			return 0;
		}

		GLuint texture;
		glGenTextures( 1, &texture );

		glBindTexture( GL_TEXTURE_2D, texture );

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image );
		SOIL_free_image_data( image );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		TEXTURE_LIST[path] = texture;

		return texture;
	}

	GLuint Graphics::LoadMesh(std::string path)
	{
		// Need to add code to assign texture GLuint to model
		//path = "Assets/Meshes/" + path;

		if( MESH_LIST.find(path) != MESH_LIST.end() )
		{
			return MESH_LIST.at(path);
		}

		std::string fullpath = "Assets/Meshes/" + path;
	
		Assimp::Importer importer;
		scene = importer.ReadFile( fullpath,aiProcessPreset_TargetRealtime_MaxQuality );

		if( !scene)
		{
			printf("%s\n", importer.GetErrorString());
			return false;
		}

		std::cout << "Import of scene %s succeeded: <" << path.c_str() << ">" << std::endl;

		GLuint buff, mesh;
		unsigned vsize = 0;

		for( unsigned k = 0; k < scene->mNumMeshes; ++k )
			vsize += scene->mMeshes[k]->mNumFaces * varray_size * 3;

		float* vertices = (float*)malloc(sizeof(float) * vsize);		// #faces * #attributes * #tri vertices
				
		for( unsigned k = 0; k < scene->mNumMeshes; ++k )
		{
			unsigned index = 0;			

			for( unsigned i = 0; i < scene->mMeshes[k]->mNumFaces; ++i )
			{
				aiFace face = scene->mMeshes[k]->mFaces[i];

				for( unsigned j = 0; j < face.mNumIndices; ++j, index+=varray_size )
				{
					if( scene->mMeshes[k]->HasPositions() )
					{
						vertices[index]		= scene->mMeshes[k]->mVertices[face.mIndices[j]].x;
						vertices[index+1]	= scene->mMeshes[k]->mVertices[face.mIndices[j]].y;
						vertices[index+2]	= scene->mMeshes[k]->mVertices[face.mIndices[j]].z;
					}

					if( scene->mMeshes[k]->HasNormals() )
					{
						vertices[index+3]	= scene->mMeshes[k]->mNormals[face.mIndices[j]].x;
						vertices[index+4]	= scene->mMeshes[k]->mNormals[face.mIndices[j]].y;
						vertices[index+5]	= scene->mMeshes[k]->mNormals[face.mIndices[j]].z;
					}

					if( scene->mMeshes[k]->HasTextureCoords(0) )
					{
						vertices[index+6]	= scene->mMeshes[k]->mTextureCoords[0][face.mIndices[j]].x;
						vertices[index+7]	= scene->mMeshes[k]->mTextureCoords[0][face.mIndices[j]].y;
					}

					
				}
			}
		}		

		ProcessVertexData(vertices,mesh,buff,vsize);
		MESH_LIST[path] = mesh;
		MESH_VERTICES[mesh] = vsize/varray_size;


		return mesh;
	}

	void Graphics::ProcessVertexData(float *vertices, GLuint& mesh, GLuint& buff, unsigned size)
	{
		if( SHADER_PROGRAMS.size() == 0 )
			return;

		glGenVertexArrays( 1, &mesh );
		glBindVertexArray( mesh );
		
		glGenBuffers( 1, &buff );
		glBindBuffer( GL_ARRAY_BUFFER, buff );
		glBufferData( GL_ARRAY_BUFFER, size*sizeof(float), vertices, GL_DYNAMIC_DRAW );
		//glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_DYNAMIC_DRAW );

		// Specify the layout of the vertex data
		
		glBindBuffer( GL_ARRAY_BUFFER, buff );

			GLint posAttrib = glGetAttribLocation( SHADER_PROGRAMS.at(SH_Phong), "position" );
			glEnableVertexAttribArray( posAttrib );
			glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0 );

			GLint normAttrib = glGetAttribLocation( SHADER_PROGRAMS.at(SH_Phong), "normal" );
			glEnableVertexAttribArray( normAttrib );
			glVertexAttribPointer( normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)( 3*sizeof(float) ) );

			GLint texAttrib = glGetAttribLocation( SHADER_PROGRAMS.at(SH_Phong), "texcoord" );
			glEnableVertexAttribArray( texAttrib );
			glVertexAttribPointer( texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float) ) );


		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER,0);
	}


	void Graphics::LoadFontmap(std::string path)
	{
		GLuint texture;
		glGenTextures( 1, &texture );
	
		int width, height;
		unsigned char* image;
	
		glBindTexture( GL_TEXTURE_2D, texture );
		image = SOIL_load_image( path.c_str(), &width, &height, 0, SOIL_LOAD_RGB );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image );
		SOIL_free_image_data( image );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		FONTMAPS.push_back(texture);
	}

	void Graphics::AddComponent(Model* m)
	{
		MODEL_LIST.push_back(m);
	}

	void Graphics::AddComponent(Light* l)
	{
		LIGHT_LIST.push_back(l);
	}

	void Graphics::RemoveComponent(Model* m)
	{
		auto it_s = MODEL_LIST.begin();
		auto it_e = MODEL_LIST.end();
		for( ; it_s != it_e; ++it_s )
		{
			if( *it_s == m )
			{
				MODEL_LIST.erase(it_s);
				return;
			}
		}
	}

	void Graphics::RemoveComponent(Light* l)
	{
		auto it_s = LIGHT_LIST.begin();
		auto it_e = LIGHT_LIST.end();
		for( ; it_s != it_e; ++it_s )
		{
			if( *it_s == l )
			{
				LIGHT_LIST.erase(it_s);
				return;
			}
		}
	}

	GLint Graphics::CreateShaderProgram(std::string _vertSource, std::string _fragSource)
	{
		GLuint vertShader = LoadShader(_vertSource, GL_VERTEX_SHADER);
		GLuint fragShader = LoadShader(_fragSource, GL_FRAGMENT_SHADER);

		VERTEX_SHADERS.push_back(vertShader);
		FRAGMENT_SHADERS.push_back(fragShader);

		GLuint shaderProgram = glCreateProgram();
		glAttachShader( shaderProgram, vertShader );
		glAttachShader( shaderProgram, fragShader );
		glBindFragDataLocation( shaderProgram, 0, "outColor" );
		glLinkProgram( shaderProgram );	

		SHADER_PROGRAMS.push_back(shaderProgram);

		return shaderProgram;
	}


	GLint Graphics::LoadShader(std::string shader_filename, GLenum shader_type)
	{
		std::ifstream shaderFile( shader_filename );
		std::stringstream shaderData;
		shaderData << shaderFile.rdbuf();  //Loads the entire string into a string stream.
		shaderFile.close();
		const std::string &shaderString = shaderData.str(); //Get the string stream as a std::string.

		const char* source = shaderString.c_str(); 

		GLuint shader_resource = glCreateShader( shader_type );
		glShaderSource( shader_resource, 1, (const GLchar**)(&source), NULL );

		glCompileShader( shader_resource );

		GLint status;
		glGetShaderiv( shader_resource, GL_COMPILE_STATUS, &status ); 

		if( status != GL_TRUE )
		{
			char buffer[512];
			glGetShaderInfoLog( shader_resource, 512, NULL, buffer ); 
			std::cout << buffer << std::endl;
		}

		return shader_resource;
	}

	void Graphics::SetupFonts()
	{
		font = new FTGLPixmapFont("Assets/Fonts/arial.ttf");

		if( font->Error() )
		{
			std::cout << "Font didn't load properly" << std::endl;
			return;
		}
		
		font->FaceSize(32);
	}

	void Graphics::UpdateScreenDims(int x, int y, int w, int h)
	{
		screen_width_i	= w;
		screen_height_i	= h;
		screen_width	= (float)w;
		screen_height	= (float)h;

		glViewport(0, 0, w, h);
	}

	void Graphics::SortModels(float dt)
	{
		std::sort( MODEL_LIST.begin(), MODEL_LIST.end(), ZSorting(*this) );
	}

	void Graphics::InitializeCamera()
	{
		cam_pos		= vec3(0.0f,0.0f,4.0f);
		cam_look	= vec3(0.0f, 0.0f, -1.0f);
		cam_up		= vec3(0.0f, 1.0f, 0.0f);
		cam_rot		= quat();
	}

	void Graphics::UpdateCamera(float dt)
	{
		vec3 move(0,0,0); 
		float move_speed = 0.01f*dt;
		
		if( Input::GetInstance()->GetKey(SDLK_w) == true )
			move -= vec3(0.0f,0,move_speed);

		if( Input::GetInstance()->GetKey(SDLK_s) == true )
			move += vec3(0.0f,0,move_speed);

		if( Input::GetInstance()->GetKey(SDLK_a) == true )
			move -= vec3(move_speed,0,0);

		if( Input::GetInstance()->GetKey(SDLK_d) == true )
			move += vec3(move_speed,0,0);

		if( Input::GetInstance()->GetKey(SDLK_r) == true )
			move += vec3(0,move_speed,0);

		if( Input::GetInstance()->GetKey(SDLK_f) == true )
			move -= vec3(0,move_speed,0);

		cam_pos += cam_rot * move;

		cam_look = cam_rot * vec3(0,0,-1);

		viewMat = glm::lookAt( cam_pos, cam_pos + cam_look, cam_up );
		projMat = glm::perspective( 45.0f, screen_width / screen_height, 0.1f, 1000.0f );
	}

	void Graphics::UpdateCameraRotation(float x, float y)
	{
		CameraYaw(x);
		CameraPitch(y);				
	}

	void Graphics::CameraPitch(float angle)
	{
		pitch += angle;

		if( pitch < -90.0f )
			pitch = -90.0f + EPSILON;
		else if( pitch > 90.0f )
			pitch = 90.0f - EPSILON;

		cam_rot = glm::normalize( glm::rotate(cam_rot, pitch, vec3(-1,0,0)) );
	}

	void Graphics::CameraYaw(float angle)
	{
		cam_rot = glm::normalize( glm::rotate(cam_rot, angle, vec3(0,-1,0)) );

		cam_rot.x = 0;
		cam_rot.z = 0;
		float mag = 1.0f / sqrt(cam_rot.w*cam_rot.w + cam_rot.y*cam_rot.y);
		cam_rot.w *= mag;
		cam_rot.y *= mag;
	}

	void Graphics::CameraRoll(float angle)
	{
		cam_rot = glm::normalize( glm::rotate(cam_rot, angle, cam_look) );
	}

	void Graphics::MouseSelectEntity(float x, float y, bool pressed)
	{
		if( pressed == false )
		{
			SelectedEntity = nullptr;
			return;
		}

		for( unsigned i = 0; i < MODEL_LIST.size(); ++i )
		{
			vec3 position   = MODEL_LIST[i]->GetTransform()->Position;

			vec4 pos;
			pos = viewMat * vec4(position,1.0f);
			pos = projMat * pos;

			pos.x /= pos.z;
			pos.y /= pos.z;

			pos.x = (pos.x + 1.0f) * screen_width / 2.0f;
			pos.y = (pos.y + 1.0f) * screen_height / 2.0f;
			pos.y = screen_height - pos.y;

			if( pos.x < 0 || pos.x > screen_width || pos.y < 0 || pos.y > screen_height )
				continue;

			float dist		= glm::distance(position, cam_pos);
			float radius	= 100.0f;//1.0f / dist * 1000.0f;

			vec2 mouse_xy = vec2(x,y);
			vec2 obj_xy   = vec2(pos.x,pos.y);

			if( glm::distance(mouse_xy,obj_xy) < radius )
			{
				SelectedEntity = MODEL_LIST[i]->Owner;
				return;
			}
		}
	}

	void Graphics::UpdateLightPos(float x = 0, float y = 0)
	{
		if( SelectedEntity == nullptr )
			return;

		vec3 move(0,0,0); 		

		float move_speed = 0.05f*x;
		move += vec3(move_speed,0,0);

		move_speed = 0.05f*y;	

		if( Input::GetInstance()->GetKey(SDLK_RSHIFT) == true || Input::GetInstance()->GetKey(SDLK_LSHIFT) == true )
		{
			move -= vec3(0,move_speed,0);

			SelectedEntity->GetTransform()->Position += move;
		}
		else
		{
			move += vec3(0,0,move_speed);		

			quat turn = cam_rot;
			turn.x = 0;
			turn.z = 0;
			float mag = 1.0f / sqrt(turn.w*turn.w + turn.y*turn.y);
			turn.w *= mag;
			turn.y *= mag;

			SelectedEntity->GetTransform()->Position += turn * move;
		}		
	}

	Graphics::~Graphics()
	{
		for( GLuint i : SHADER_PROGRAMS )
			glDeleteProgram(i);

		for( GLuint i : VERTEX_SHADERS )
			glDeleteProgram(i);

		for( GLuint i : FRAGMENT_SHADERS )
			glDeleteProgram(i);

		glDeleteBuffers( 1, &buffCube );
		glDeleteBuffers( 1, &buffQuad );

		glDeleteVertexArrays( 1, &meshCube );
		glDeleteVertexArrays( 1, &meshQuad );

		glDeleteFramebuffers( 1, &screen_fbo );
		glDeleteFramebuffers( 1, &shadow_fbo );
	}
}