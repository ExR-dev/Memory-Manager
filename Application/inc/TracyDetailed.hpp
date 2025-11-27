#pragma once

#ifdef TRACY_DETAILED
#define ZoneScopedX						ZoneScoped
#define ZoneScopedXN( name )			ZoneScopedN( name )
#define ZoneScopedXC( color )			ZoneScopedC( color )
#define ZoneScopedXNC( name, color )	ZoneScopedNC( name, color )

#define ZoneTextX( txt, size )				ZoneText( txt, size )
#define ZoneTextXV( varname, txt, size )	ZoneTextV( varname, txt, size )
#define ZoneTextXF( fmt, ... )				ZoneTextF( fmt, ##__VA_ARGS__ )
#define ZoneTextXVF( varname, fmt, ... )	ZoneTextVF( varname, fmt, ##__VA_ARGS__ )
#define ZoneNameX( txt, size )				ZoneName( txt, size )
#define ZoneNameXV( varname, txt, size )	ZoneNameV( varname, txt, size )
#define ZoneNameXF( fmt, ... )				ZoneNameF( fmt, ##__VA_ARGS__ )
#define ZoneNameXVF( varname, fmt, ... )	ZoneNameVF( varname, fmt, ##__VA_ARGS__ ) 
#define ZoneColorX( color ) 				ZoneColor( color ) 
#define ZoneColorXV( varname, color )		ZoneColorV( varname, color )
#define ZoneValueX( value ) 				ZoneValue( value ) 
#define ZoneValueXV( varname, value ) 		ZoneValueV( varname, value ) 
#define ZoneIsActiveX						ZoneIsActive
#define ZoneIsActiveXV( varname )			ZoneIsActiveV( varname )

#define ZoneScopedXNCD( name, color )			ZoneScopedNCD( name, color )
#define ZoneNamedXNCD( varname, name, color )	ZoneNamedNCD( varname, name, color )

#define ZoneNamedX( varname, active ) 					ZoneNamed( varname, active ) 
#define ZoneNamedXN( varname, name, active ) 			ZoneNamedN( varname, name, active ) 
#define ZoneNamedXC( varname, color, active ) 			ZoneNamedC( varname, color, active ) 
#define ZoneNamedXNC( varname, name, color, active )	ZoneNamedNC( varname, name, color, active ) 

#define ZoneTransientX( varname, active ) 					ZoneTransient( varname, active ) 
#define ZoneTransientXN( varname, name, active ) 			ZoneTransientN( varname, name, active ) 
#define ZoneTransientXNC( varname, name, color, active )	ZoneTransientNC( varname, name, color, active ) 
#else
#define ZoneScopedX( )	
#define ZoneScopedXN( name )
#define ZoneScopedXC( color )	
#define ZoneScopedXNC( name, color )	

#define ZoneTextX( txt, size )		
#define ZoneTextXV( varname, txt, size )	
#define ZoneTextXF( fmt, ... )				
#define ZoneTextXVF( varname, fmt, ... )	
#define ZoneNameX( txt, size )				
#define ZoneNameXV( varname, txt, size )	
#define ZoneNameXF( fmt, ... )				
#define ZoneNameXVF( varname, fmt, ... )	
#define ZoneColorX( color ) 				
#define ZoneColorXV( varname, color )		
#define ZoneValueX( value ) 				
#define ZoneValueXV( varname, value ) 		
#define ZoneIsActiveX( )						
#define ZoneIsActiveXV( varname )			

#define ZoneScopedXNCD( name, color )
#define ZoneNamedXNCD( varname, name, color )

#define ZoneNamedX( varname, active ) 					
#define ZoneNamedXN( varname, name, active ) 			
#define ZoneNamedXC( varname, color, active ) 			
#define ZoneNamedXNC( varname, name, color, active )	

#define ZoneTransientX( varname, active ) 				
#define ZoneTransientXN( varname, name, active ) 		
#define ZoneTransientXNC( varname, name, color, active )
#endif