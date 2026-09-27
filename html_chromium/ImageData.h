#pragma once

#include <algorithm>

#include "cef_start.h"
#include "include/cef_base.h"
#include "include/base/cef_lock.h"
#include "cef_end.h"

class ImageData
{
public:
	ImageData()
		: m_Dirty( false )
		, m_Wide( 0 )
		, m_Tall( 0 )
		, m_Size( 0 )
		, m_Data( nullptr )
	{}

	~ImageData()
	{
		delete[] m_Data;
	}

	void Lock()
	{
		m_Lock.Acquire();
	}

	void Unlock()
	{
		m_Lock.Release();
	}

	bool ResizeData(int wide, int tall)
	{
		size_t size = static_cast<size_t>( wide ) * static_cast<size_t>( tall ) * 4;
		bool resized = size > m_Size;
		if ( resized )
		{
			unsigned char* old_Data = m_Data;
			m_Size = size;
			m_Data = new unsigned char[m_Size];
			delete[] old_Data;
		}

		m_Wide = wide;
		m_Tall = tall;
		return resized;
	}

	void Blit( const unsigned char* data, int x, int y, int width, int height )
	{
		if ( x + width > m_Wide || y + height > m_Tall ) {
			LOG(WARNING) << "Invalid Blit!";
			return;
		}

		for ( int SrcY = 0; SrcY < height; SrcY++ )
		{
			// TODO: See if AVX copy or SIMD can speed this up?
			memcpy( &m_Data[( SrcY + y ) * m_Wide * 4 + x * 4], &data[( SrcY + y ) * m_Wide * 4 + x * 4], width * 4 );
		}
		m_Dirty = true;
	}

	void CopyImage( const unsigned char* data, int x, int y, int w, int h )
	{
		if ( w < 0 || h < 0 || !data || !m_Data )
		{
			return;
		}

		const int dstX = std::max( 0, x );
		const int dstY = std::max( 0, y );
		const int srcX = std::max( 0, -x );
		const int srcY = std::max( 0, -y );

		const int copyW = std::min( w - srcX, m_Wide - dstX );
		const int copyH = std::min( h - srcY, m_Tall - dstY );

		if ( copyW > 0 && copyH > 0 )
		{
			for ( int row = 0; row < copyH; ++row )
			{
				const size_t dstOffset = static_cast<size_t>( ( dstY + row ) * m_Wide + dstX ) * 4;
				const size_t srcOffset = static_cast<size_t>( ( srcY + row ) * w + srcX ) * 4;
				memcpy( &m_Data[dstOffset], &data[srcOffset], static_cast<size_t>( copyW ) * 4 );
			}

			m_Dirty = true;
		}
	}

	void SetData( const unsigned char* data, int wide, int tall )
	{
		if ( m_Wide != wide || m_Tall != tall )
		{
			// TODO: Add warning log here for invalid data set. Call ResizeData first.
			ResizeData(wide, tall);
		}

		memcpy( m_Data, data, static_cast<size_t>( wide ) * static_cast<size_t>( tall ) * 4 );
		m_Dirty = true;
	}

	const unsigned char* GetData( int& wide, int& tall )
	{
		wide = m_Wide;
		tall = m_Tall;
		return m_Data;
	}

	bool IsDirty()
	{
		return m_Dirty;
	}

	void SetDirty( bool dirty )
	{
		m_Dirty = dirty;
	}

	ImageData( const ImageData& ) = delete;
	ImageData& operator=( const ImageData& ) = delete;

private:
	friend class ChromiumBrowser;
	bool m_Dirty;

	int m_Wide;
	int m_Tall;
	size_t m_Size;

	base::Lock m_Lock;
	unsigned char* m_Data;
};
