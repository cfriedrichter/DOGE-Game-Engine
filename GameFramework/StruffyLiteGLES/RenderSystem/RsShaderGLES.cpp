////////////////////////////////////////////////////////////////////////////////
// RsShaderWinGL.cpp

// Includes
#include "RsGLES.h"
#include "BaArchive.h"
#include "BtMemory.h"
#include "BtString.h"
#include "RsShaderGLES.h"
#include "RsMaterialGLES.h"
#include "RsIndexBufferGLES.h"
#include "RsImpl.h"
#include "ErrorLog.h"
#include <new>

static BtU32 m_currentProgram = 0;

////////////////////////////////////////////////////////////////////////////////
// Statics

void getError( int objectHandle )
{
	GLint blen = 0;
	GLsizei slen = 0;
    
	glGetShaderiv( objectHandle, GL_INFO_LOG_LENGTH , &blen);
	if (blen > 1)
	{
		GLchar* compiler_log = (GLchar*)malloc(blen);
		glGetShaderInfoLog(objectHandle, blen, &slen, compiler_log);
		ErrorLog::Fatal_Printf( "%s\n", compiler_log );
		free (compiler_log);
	}
}

////////////////////////////////////////////////////////////////////////////////
// FixPointers

void RsShaderWinGL::FixPointers( BtU8 *pFileData, BaArchive *pArchive )
{
    GLenum error = 0;
    GLint status;
    
    // Set the file data
    m_pFileData = (BaShaderFileData*) pFileData;
    m_pArchive = pArchive;
    
    BtChar *pData = (BtChar*) m_pFileData;
    BtU32 i = 0;
    
    for( BtU32 shaderIndex=0; shaderIndex<m_pFileData->m_count; shaderIndex = shaderIndex + 2 )
    {
        GLuint program = glCreateProgram();
        
        m_program[i] = program;
        
        BaShader &vertexShaderData = m_pFileData->m_shaders[shaderIndex + 0];
        BaShader &pixelShaderData  = m_pFileData->m_shaders[shaderIndex + 1];
        
        BtU32 vertexShader   = glCreateShader(GL_VERTEX_SHADER);
        BtU32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        
        const GLchar *vertexText   = (const GLchar *)(pData + vertexShaderData.m_offset );
        const GLchar *fragmentText = (const GLchar *)(pData + pixelShaderData.m_offset);
        
        glShaderSource( vertexShader, 1, &vertexText, (const GLint*)&vertexShaderData.m_size );
        glShaderSource( fragmentShader, 1, &fragmentText, (const GLint*)&pixelShaderData.m_size);
        
        ErrorLog::Printf( "Compiling vertex shader %d\n", shaderIndex );
      
        glCompileShader(vertexShader);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status );
        if( status == 0 )
        {
            getError( vertexShader );
        }
        
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status );
        if( status == 0 )
        {
            ErrorLog::Printf( "Error compiling pixel shader %d = %s\n", shaderIndex, fragmentText );
            getError( fragmentShader );
        }
        //else
        //{
        //    ErrorLog::Printf( "Compiled pixel shader %d\n", i );
        //}
        
        glAttachShader( m_program[i], vertexShader );
        glAttachShader( m_program[i], fragmentShader );
        glLinkProgram( m_program[i] );
        glUseProgram( m_program[i] );
        
        if( i != RsShaderPassThrough )
        {
            m_handles[i][RsHandles_Light0Direction] = glGetUniformLocation(m_program[i], "s_lightDirection");
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_LightAmbient] = glGetUniformLocation(m_program[i], "s_lightAmbient");
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_Colour] = glGetUniformLocation(m_program[i], "s_materialColour");
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_World] = glGetUniformLocation(m_program[i], "s_m4World");
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_WorldViewScreen] = glGetUniformLocation( m_program[i], "s_m4WorldViewScreen" );
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_WorldViewInverseTranspose] = glGetUniformLocation( m_program[i], "s_m4WorldViewInverseTranspose" );
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_ViewInverseTranspose] = glGetUniformLocation( m_program[i], "s_m4ViewInverseTranspose" );
            error = glGetError();
            (void)error;
        }
        if (i == RsProjectedTexture )
        {
            m_handles[i][RsHandles_World] = glGetUniformLocation(m_program[i], "s_m4World");
            error = glGetError();
            (void)error;
            
            m_handles[i][RsHandles_TextureMatrix0] = glGetUniformLocation(m_program[i], "s_m4ProjectionTexture");
            error = glGetError();
            (void)error;
            
            MtMatrix4 m4Matrix;
            m4Matrix.SetIdentity();
            glUniformMatrix4fv( m_handles[i][RsHandles_TextureMatrix0], 1, GL_FALSE, (const GLfloat*)&m4Matrix );
            
            error = glGetError();
            (void)error;
            int a = 0;
            a++;
        }
        
        glBindAttribLocation( m_program[i], 0, "position" );
        error = glGetError();
        (void)error;
        
        glBindAttribLocation( m_program[i], 1, "normal" );
        error = glGetError();
        (void)error;
        
        glBindAttribLocation( m_program[i], 4, "colour" );
        error = glGetError();
        (void)error;
        
        glBindAttribLocation( m_program[i], 5, "tex0" );
        error = glGetError();
        (void)error;
        
        if( i == RsShaderSkin )
        {
            m_handles[i][RsHandles_BoneMatrix] = glGetUniformLocation( m_program[i], "s_m4BoneMatrices" );
            
            error = glGetError();
            (void)error;
            
            glBindAttribLocation( m_program[i], 6, "weights" );
            
            error = glGetError();
            (void)error;
            
            glBindAttribLocation( m_program[i], 7, "boneIndex" );
            
            error = glGetError();
            (void)error;
        }
        glLinkProgram( m_program[i] );
        
        error = glGetError();
        (void)error;
        
        glUseProgram( BtNull );
        
        ++i;
    }
}

////////////////////////////////////////////////////////////////////////////////
// CreateOnDevice

void RsShaderWinGL::CreateOnDevice()
{
}

////////////////////////////////////////////////////////////////////////////////
// pDuplicate

RsShader* RsShaderWinGL::pDuplicate()
{
	// Allocate the memory
	BtU8* pMemory = BtMemory::Allocate( sizeof(RsShaderWinGL) );
    
	// Create the class
	RsShaderWinGL* pShader = new( pMemory ) RsShaderWinGL;
    
	// Copy the details
	BtMemory::Copy( pShader, this, sizeof( RsShaderWinGL ) );
    
	// Flag as a duplicate
	pShader->m_isDuplicate = BtTrue;
    
	// Add the duplicate
	m_pArchive->AddDuplicate( pShader );
    
	// Create the shader on the device
	pShader->CreateOnDevice();
    
	// Return the memory
	return pShader;
}

////////////////////////////////////////////////////////////////////////////////
// SetShaderParam

void RsShaderWinGL::SetParams(BtU8 *paramData, BtU32 length)
{
}

////////////////////////////////////////////////////////////////////////////////
// SetAmbient

void RsShaderWinGL::SetAmbient( const RsColour &colour )
{
    MtVector4 v4Ambient( colour.Red(), colour.Green(), colour.Blue(), 1.0f );
    
    // Set the ambient lights
    m_floats[RsHandles_LightAmbient]= v4Ambient;
}

////////////////////////////////////////////////////////////////////////////////
// SetDirectionalLight

void RsShaderWinGL::SetDirectionalLight( const MtVector3 &v3Direction )
{
    // Set the light direction
    MtVector4 v4Direction( v3Direction.x, v3Direction.y, v3Direction.z, 1.0f );
    m_floats[RsHandles_Light0Direction] = v4Direction;
}

////////////////////////////////////////////////////////////////////////////////
// Remove

void RsShaderWinGL::Remove()
{
}

////////////////////////////////////////////////////////////////////////////////
// RemoveFromDevice

void RsShaderWinGL::RemoveFromDevice()
{
}

////////////////////////////////////////////////////////////////////////////////
// SetTechnique

void RsShaderWinGL::SetTechnique( const BtChar* pTechniqueName )
{
	GLenum error;
	error = glGetError();
	(void)error;
    
	// http://en.wikibooks.org/wiki/OpenGL_Programming/Stencil_buffer
	if (BtStrCompare(pTechniqueName, "RsShaderAdditive") == BtTrue)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		m_currentProgram = RsShaderTextured;
	}
	else if (BtStrCompare(pTechniqueName, "lensflare") == BtTrue)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc( GL_ONE, GL_ONE );
		m_currentProgram = RsShaderPassThrough;
	}
	else if (BtStrCompare(pTechniqueName, "RsShadow") == BtTrue)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		m_currentProgram = RsShadow;
	}
	else if (BtStrCompare(pTechniqueName, "RsProjectedShadow") == BtTrue)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		m_currentProgram = RsProjectedTexture;
	}
	else if( BtStrCompare( pTechniqueName, "RsShaderT2" ) == BtTrue )
	{
		glDisable( GL_CULL_FACE );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderPassThrough;
	}
	else if( BtStrCompare( pTechniqueName, "RsShaderTG2" ) == BtTrue )
	{
		glDisable( GL_CULL_FACE );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderPassThrough;
	}
	else if( BtStrCompare( pTechniqueName, "RsShaderT" ) == BtTrue )
	{
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderTextured;
	}
	else if( BtStrCompare( pTechniqueName, "RsShaderT" ) == BtTrue )
	{
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderTextured;
	}
	else if( BtStrCompare( pTechniqueName, "RsShaderZL" ) == BtTrue )
	{
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glEnable( GL_CULL_FACE );
		glFrontFace(GL_CW);
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderLit;
	}
	else if (BtStrCompare(pTechniqueName, "RsShaderZLTGD"))
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderLitTextured;
	}
	else if ( BtStrCompare(pTechniqueName, "RsShaderZLT") ||
              BtStrCompare(pTechniqueName, "RsShaderZLTG") ||
              BtStrCompare(pTechniqueName, "RsShaderZTG"))
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderLitTextured;
	}
	else if( BtStrCompare( pTechniqueName, "RsShaderZLTS" ) )
	{
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_CULL_FACE );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
		// Use The Program Object Instead Of Fixed Function OpenGL
		m_currentProgram = RsShaderSkin;
	}
	else
	{
		//ErrorLog::Fatal_Printf( "Shader %s not implemented", pTechniqueName );
        int a=0;
        a++;
	}
    
	glUseProgram( m_program[m_currentProgram] );
    
	switch( m_currentProgram )
	{
        case RsShaderLit:
        case RsShaderLitTextured:
        case RsShaderSkin:
            SetLights( m_currentProgram );
            break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// SetMatrix

void RsShaderWinGL::SetMatrix( const BtChar* pMatrix, const MtMatrix4& m4Matrix )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetOrthgonalProjection

void SetOrthgonalProjection(float width, float height, float zNear, float zFar)
{
}

////////////////////////////////////////////////////////////////////////////////
// SetPerspectiveProjection

void SetPerspectiveProjection(float fovy, float aspect, float zNear, float zFar)
{
}

////////////////////////////////////////////////////////////////////////////////
// SetCamera

void RsShaderWinGL::SetCamera( const RsCamera &camera )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetMatrix

void RsShaderWinGL::SetMatrix( RsHandles handle, const MtMatrix4& m4Matrix )
{
	if( handle == RsHandles_TextureMatrix0 )
	{
		GLenum error;
        
		glUseProgram( m_program[RsProjectedTexture] );
        
		error = glGetError();
        
		glUniformMatrix4fv( m_handles[RsProjectedTexture][handle], 1, GL_FALSE, (const GLfloat*)&m4Matrix);
        
		error = glGetError();
        
		int a = 0;
		a++;
	}
	else
	{
		glUniformMatrix4fv( m_handles[m_currentProgram][handle], 1, GL_FALSE, (const GLfloat*)&m4Matrix );
	}
}

// http://www.sjbaker.org/steve/omniv/opengl_lighting.html
// http://www.devmaster.net/forums/archive/index.php/t-348.html

////////////////////////////////////////////////////////////////////////////////
// SetFloats

void RsShaderWinGL::SetFloats( RsHandles handle, const BtFloat* pArray, BtU32 nCount )
{
	m_floats[handle] = MtVector4( pArray[0], pArray[1], pArray[2], 1.0f );
}

////////////////////////////////////////////////////////////////////////////////
// SetMatrix

void RsShaderWinGL::SetMatrix( const BtChar* pMatrixName, const MtMatrix4* pMatrix, BtU32 nCount )
{
	glUniformMatrix4fv( m_handles[RsShaderSkin][RsHandles_BoneMatrix], nCount, GL_FALSE, (const GLfloat*)pMatrix );
    
	GLenum error = glGetError();
	(void)error;
    
	int a=0;
	a++;
}

////////////////////////////////////////////////////////////////////////////////
// SetColour

void RsShaderWinGL::SetColour( const BtChar* pColourName, const RsColour& colour )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetFloats

void RsShaderWinGL::SetFloats( const BtChar* pArrayName, const BtFloat* pArray, BtU32 nCount )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetFloat

void RsShaderWinGL::SetFloat( const BtChar* pName, BtFloat value )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetInteger

void RsShaderWinGL::SetInteger( const BtChar* pName, BtU32 value )
{
}

////////////////////////////////////////////////////////////////////////////////
// Commit

void RsShaderWinGL::Commit()
{
}

////////////////////////////////////////////////////////////////////////////////
// DrawPrimitive

void RsShaderWinGL::Draw( const RsPrimitive *pPrimitive )
{
	Commit();
    
	RsPrimitive* pPrimitives = (RsPrimitive*) pPrimitive;
    
	// Draw vertex
	glDrawArrays( pPrimitives->m_primitiveType, 0, pPrimitives->m_numVertex );
}

////////////////////////////////////////////////////////////////////////////////
// Draw

void RsShaderWinGL::Draw( const RsIndexedPrimitive *pPrimitive )
{
	Commit();
    
	RsIndexedPrimitive* pPrimitives = (RsIndexedPrimitive*) pPrimitive;
    
	GLenum primType = pPrimitives->m_primitiveType;
    
    BtU32 indexType = GL_UNSIGNED_SHORT;
    BtU32 indexSize = 2;
    
    if (pPrimitives->m_indexType == RsIndexBufferWinGL::IndType_Long)
    {
        indexType = GL_UNSIGNED_INT;
        indexSize = 4;
    }
    
    // Draw vertex
	glDrawElements( primType,
                   pPrimitives->m_numIndices,
                   GL_UNSIGNED_SHORT,
                   GL_BUFFER_OFFSET( pPrimitives->m_startIndex * 2 ) );
}

////////////////////////////////////////////////////////////////////////////////
// SetTexture

void RsShaderWinGL::SetTexture( const BtChar* pName, const RsTexture* pTexture )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetTexture

void RsShaderWinGL::SetTexture( const BtChar* pName )
{
}

////////////////////////////////////////////////////////////////////////////////
// SetMaterial

void RsShaderWinGL::SetMaterial( RsMaterial* pMaterial )
{
	RsMaterialWinGL* pMaterialWinGL = (RsMaterialWinGL*) pMaterial;
    
	BtU32 flag = pMaterialWinGL->GetFlags() & RsMaterial_Lit;
    
	SetFloat( "s_materialFlags", (BtFloat) flag );
	SetColour( "s_v4MaterialColour", pMaterialWinGL->GetDiffuseColour() );
    
	//glUniform1i( m_sampler, 0);	// Texture Unit 0
    
	// http://lwjgl.org/wiki/index.php?title=GLSL_Tutorial:_Texturing
	// Set the textures
	for( BtU32 iTexture=0; iTexture<1; ++iTexture )
	{
		// Cache each texture
		RsTextureWinGL *pTexture = (RsTextureWinGL*)pMaterialWinGL->GetTexture(iTexture);
        
		if( pTexture != BtNull )
		{
			// http://www.opengl.org/wiki/Sampler_(GLSL)
			glActiveTexture( GL_TEXTURE0 );
			pTexture->SetTexture();
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0 );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// SetLights

void RsShaderWinGL::SetLights(BtU32 index)
{
	// Set the light direction
	MtVector4 v4Direction = m_floats[RsHandles_Light0Direction];
	glUniform4fv( m_handles[index][RsHandles_Light0Direction], 1, v4Direction );
    
	// Set the ambient lights
	MtVector4 v4Ambient = m_floats[RsHandles_LightAmbient];
	v4Ambient.w = 0;
	glUniform4fv( m_handles[index][RsHandles_LightAmbient], 1, v4Ambient );
    
	int a=0;
	a++;
}
