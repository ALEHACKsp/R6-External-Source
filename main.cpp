#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <WinUser.h>
#include <tlhelp32.h>
#include <iostream>
#include <Psapi.h>
#include <string>
#include <thread>
#include <TlHelp32.h>
#include <string>
#include <cstdarg>
#include <cmath>
#include <chrono>
#include <thread>
#include "Globals.h"
#include "GUI.h"
#include "Vector.h"
#include "Offsets.h"

HWND hwnd;
HANDLE hProcess;
uint32_t pid;
uint64_t base_address;

template<typename T>
T RPM ( uintptr_t address )
{
    T buffer;
    ReadProcessMemory ( hProcess , ( LPCVOID ) address , &buffer , sizeof ( T ) , NULL );
    return buffer;
}

template<typename T>
void WPM ( uintptr_t address , T buffer )
{
    WriteProcessMemory ( hProcess , ( LPVOID ) address , &buffer , sizeof ( buffer ) , NULL );
}


DWORD GetPID ( const char* ProcessName ) {
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof ( processInfo );


    HANDLE processesSnapshot = CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS , NULL );
    if ( processesSnapshot == INVALID_HANDLE_VALUE )
        return 0;

    Process32First ( processesSnapshot , &processInfo );
    if ( !strcmp ( processInfo.szExeFile , ProcessName ) )
    {
        CloseHandle ( processesSnapshot );
    }

    while ( Process32Next ( processesSnapshot , &processInfo ) )
    {
        if ( !strcmp ( processInfo.szExeFile , ProcessName ) )
        {
            CloseHandle ( processesSnapshot );
        }
    }
    return processInfo.th32ProcessID;
}

uintptr_t GetModule ( HANDLE Handle )
{
    HMODULE hMods [ 1024 ];
    DWORD cbNeeded;
    unsigned int i;

    if ( EnumProcessModules ( Handle , hMods , sizeof ( hMods ) , &cbNeeded ) )
    {
        for ( i = 0; i < ( cbNeeded / sizeof ( HMODULE ) ); i++ )
        {
            TCHAR szModName [ MAX_PATH ];
            if ( GetModuleFileNameEx ( Handle , hMods [ i ] , szModName , sizeof ( szModName ) / sizeof ( TCHAR ) ) )
            {
                std::string wstrModName = szModName;
                std::string wstrModContain = "RainbowSix.exe";
                if ( wstrModName.find ( wstrModContain ) != std::string::npos )
                {
                    base_address = ( uintptr_t ) hMods [ i ];
                    return base_address;
                }
            }
        }
    }

    return false;
}



namespace entitys {

    uintptr_t local_player ( ) {
        auto local_player = RPM<uint64_t> ( base_address + OFFSET_PROFILE );
        if ( !local_player )
        {
            return 0;
        }
        local_player = RPM<uint64_t> ( local_player + 0x78 );
        if ( !local_player )
        {
            return 0;
        }
        local_player = RPM<uint64_t> ( local_player + 0x0 );
        if ( !local_player )
        {
            return 0;
        }
        local_player = RPM<uint64_t> ( local_player + 0x28 );

        if ( !local_player )
        {
            return 0;
        }
        return ( local_player - PROFILE_KEY );

    }



    uintptr_t EntityObject ( uint32_t index ) {

        const auto game_manager = RPM<uint64_t> ( base_address + OFFSET_GAME_MANAGER );
        if ( !game_manager ) {
            return 0;
        }
        const auto entity_list = RPM<uint64_t> ( game_manager + 0x98 ) + 0xE60F6CF8784B5E96;
        if ( !entity_list ) {
            return 0;
        }

        return RPM<uint64_t> ( entity_list + ( index * sizeof ( uintptr_t ) ) );
    }



    uint8_t EntityTeam ( uint64_t entity ) {
        uintptr_t pControl = RPM<uintptr_t> ( entity + 0x28 ) + 0xB2367ED92CF96CC8;
        if ( !pControl ) {
            return 0;
        }
        uintptr_t team_encrypted = RPM<uintptr_t> ( pControl + 0xD0 );
        if ( !team_encrypted ) {
            return 0;
        }

        uint8_t team_decrypted = RPM<uint8_t> ( team_encrypted + 0x1B2 ) + 0x24;

        if ( !team_decrypted ) {
            return 0;
        }
        return team_decrypted & 0x3F;
    }

    int32_t EntityHealth ( uint64_t local_player ) {

        uint64_t actor = RPM<uint64_t> ( local_player + 0x28 ) ^ 0xC3868E9D7D6FCC95Ui64;
        if ( !actor ) {
            return 0;
        }
        uint8_t healthComponentIndex = RPM<uint8_t> ( actor + 0x1A2 );
        if ( healthComponentIndex == -1 )
            return 0;
        uint64_t actorComponents = RPM<uint64_t> ( actor + 0xD8 );
        if ( !actorComponents ) {
            return 0;
        }
        uint64_t healthComponent = RPM<uint64_t> ( actorComponents + 0x8 * healthComponentIndex );
        if ( !healthComponent ) {
            return 0;
        }
        return RPM<int32_t> ( healthComponent + 0x168 );

    }


    int32_t LocalTeam ( uint64_t local_player ) {

        uintptr_t team_encrypted = RPM<uintptr_t> ( local_player + 0xD0 );

        if ( !team_encrypted ) {
            return 0;
        }

        int32_t team_decrypted = RPM<int> ( team_encrypted + 0x1B2 ) + 0x24;

        if ( !team_decrypted ) {
            return 0;
        }

        int32_t teamID = team_decrypted & 0x3f;

        if ( !teamID ) {
            return 0;
        }

        return teamID;
    }

}


bool is_in_game ( ) {

    uint64_t RoundManager = RPM<uint64_t> ( base_address + OFFSET_ROUND_MANAGER );

    if ( !RoundManager ) {
        return false;
    }

    int32_t Round = RPM<int32_t> ( RoundManager + 0x02E8 );

    if ( !Round ) {
        return false;
    }

    if ( Round == 2 || Round == 3 )
        return true;
    return false;

}

float GlowRainbow1 ( )
{
    static uint32_t cnt = 0;
    float freq = Globals::RGBSpeed;

    if ( cnt++ >= ( uint32_t ) -1 )
        cnt = 0;

    return std::sin ( freq * cnt + 0 ) * 0.5f + 0.5f;
}
float GlowRainbow2 ( )
{
    static uint32_t cnt = 0;
    float freq = Globals::RGBSpeed;

    if ( cnt++ >= ( uint32_t ) -1 )
        cnt = 0;

    return std::sin ( freq * cnt + 2 ) * 0.5f + 0.5f;
}
float GlowRainbow3 ( )
{
    static uint32_t cnt = 0;
    float freq = Globals::RGBSpeed;

    if ( cnt++ >= ( uint32_t ) -1 )
        cnt = 0;

    return std::sin ( freq * cnt + 4 ) * 0.5f + 0.5f;
}

auto Rainbow ( )
{
    static uint32_t cnt = 0;
    float freq = Globals::RGBSpeed;

    if ( cnt++ >= ( uint32_t ) -1 )
        cnt = 0;

    return std::make_tuple ( std::sin ( freq * cnt + 0 ) * 0.5f + 0.5f ,
        std::sin ( freq * cnt + 2 ) * 0.5f + 0.5f ,
        std::sin ( freq * cnt + 4 ) * 0.5f + 0.5f );
}


bool rapidfire ( ) {

    uint64_t local_player = entitys::local_player ( );

    uint64_t r1 = RPM<uint64_t> ( local_player + 0x90 );

    if ( !r1 ) {
        return 0;
    }

    uint64_t r2 = RPM<uint64_t> ( r1 + 0xc8 );

    if ( !r2 ) {
        return 0;
    }

    if ( Globals::rapidfire ) {
        WPM<uint8_t> ( r2 + 0x118 , 0 );

    }

}

bool CaveriaESP ( )
{

    uintptr_t game_manager = RPM<uint64_t> ( base_address + 0x6C21B48 );

    if ( !game_manager ) {
        return false;
    }

    uintptr_t entity_list = RPM<uint64_t> ( game_manager + 0x98 ) + 0xE60F6CF8784B5E96;

    if ( !entity_list ) {
        return false;
    }

    for ( int i = 0; i < 11; i++ )
    {
        uintptr_t entity_object = RPM<uint64_t> ( entity_list + i * 0x8 );

        if ( !entity_object ) {
            return 0;
        }

        uintptr_t chain2 = RPM<uint64_t> ( entity_object + 0x18 );

        if ( !chain2 ) {
            return 0;
        }

        uintptr_t chain3 = RPM<uint64_t> ( chain2 + 0xD8 );

        if ( !chain3 ) {
            return 0;
        }

        for ( auto current_component = 0x80; current_component < 0xf0; current_component += sizeof ( uintptr_t ) )
        {
            uintptr_t chain4 = RPM<uint64_t> ( chain3 + current_component );

            if ( !chain4 ) {
                return 0;
            }

            if ( RPM<uint64_t> ( chain4 ) != ( base_address + 0x4C93080 ) ) continue;
            if ( Globals::cavesp )
            {

                WPM<uint8_t> ( chain4 + 0x62A , 1 );
                WPM<uint8_t> ( chain4 + 0x62C , 1 );

            }
            else
            {
                WPM<uint8_t> ( chain4 + 0x62A , 0 );
                WPM<uint8_t> ( chain4 + 0x62C , 0 );
            }


        }
    }

}



void noanimations ( ) {

    uint64_t local_player = entitys::local_player ( );

    if ( !local_player ) {
        return;
    }

    uint64_t r1 = RPM<uint64_t> ( local_player + 0x90 );

    if ( !r1 ) {
        return;
    }

    uint64_t r2 = RPM<uint64_t> ( r1 + 0x118 );

    if ( !r2 ) {
        return;
    }

    uint64_t r3 = RPM<uint64_t> ( r2 + 0xC8 );

    if ( !r3 ) {
        return;
    }

    if ( Globals::instantanimation )
    {
        WPM<uint8_t> ( r3 + 0x384 , 0 );
    }
    else {

        WPM<uint8_t> ( r3 + 0x384 , 1 );

    }
}


bool speedMod ( )
{
    uintptr_t localplayer = entitys::local_player ( );

    if ( !localplayer ) {
        return false;
    }

    uint64_t speed = RPM<uintptr_t> ( localplayer + 0x30 );

    if ( !speed ) {
        return false;
    }

    speed = RPM<uintptr_t> ( speed + 0x31 );

    if ( !speed ) {
        return false;
    }

    speed = RPM<uintptr_t> ( speed + 0x38 );

    if ( !speed ) {
        return false;
    }

    if ( Globals::speed )
    {
        WPM<int32_t> ( speed + 0x58 , Globals::speedvalues );
    }
    else
    {
        WPM<int32_t> ( speed + 0x58 , 115 );
    }
    return true;
}


void noclip ( ) {

    const auto network_manager = RPM<std::uintptr_t> ( base_address + NETWORK_MANAGER );

    if ( !network_manager ) {
        return;
    }

    const auto network_base = RPM<std::uintptr_t> ( network_manager + 0x150 );


    if ( !network_base ) {
        return;
    }

    const auto network_list = RPM<std::uintptr_t> ( network_base + 0x8 );

    if ( !network_list ) {
        return;
    }

    if ( Globals::noclip )
    {
        WPM<float> ( network_list + 0x7F0 , -1.0f );
        WPM<float> ( network_list + 0x7F4 , -1.0f );
        WPM<float> ( network_list + 0x7F8 , -1.0f );
        WPM<float> ( network_list + 0x7FC , -1.0f );

    }
    else
    {
        WPM<float> ( network_list + 0x7F0 , 0.0003051850945f );
        WPM<float> ( network_list + 0x7F4 , 0.0003051850945f );
        WPM<float> ( network_list + 0x7F8 , 0.0003051850945f );
        WPM<float> ( network_list + 0x7FC , 0.0003051850945f );
    }


}


bool chams ( ) //such a misleading name
{
    float strength = -1.5f;
    float strength2 = 3.5f;
    uint64_t Glow_Manger = RPM<uint64_t> ( base_address + GLOW_MANAGER );

    if ( !Glow_Manger ) {
        return false;
    }

    Glow_Manger = RPM<uint64_t> ( Glow_Manger + 0xB8 );

    if ( !Glow_Manger ) {
        return false;
    }

    if ( Globals::glow )
    {

        WPM ( Glow_Manger + 0x11c , strength2 );
        if ( Globals::GlowRGB )
        {
            WPM<Vector3> ( Glow_Manger + 0xD0 , { GlowRainbow1 ( ) * 255, GlowRainbow2 ( ) * 255, GlowRainbow3 ( ) * 255 } );
        }
        else
        {
            WPM<Vector3> ( Glow_Manger + 0xD0 , { Globals::GlowCol [ 0 ] * 255, Globals::GlowCol [ 1 ] * 255, Globals::GlowCol [ 2 ] * 255 } );
        }


        if ( Globals::hands )
        {
            WPM<Vector2> ( Glow_Manger + 0x110 , { 0, 0 } );
        }
        else
        {
            WPM<Vector2> ( Glow_Manger + 0x110 , { 1, 1 } );
        }
    }
    else
    {
        WPM<Vector3> ( Glow_Manger + 0xD0 , { 0, 0, 0 } );
        WPM<Vector2> ( Glow_Manger + 0x110 , { 0, 0 } );
        WPM ( Glow_Manger + 0x11c , strength );
    }
    return true;
}



void unlockall ( ) {
    uint8_t enable [ ] = { 0x41, 0xC6, 0x46, 0x51, 0x00, 0x90 };
    // uint8_t disable [ ] = { 0x34, 0x01, 0x41, 0x88, 0x46, 0x51 };

    for ( size_t i = 0; i < 6; i++ ) {

        WPM ( base_address + OFFSET_UNLOCK_ALL + i , enable [ i ] );


    }

}


void no_flash ( )
{


    uint64_t local_player = entitys::local_player ( );

    if ( !local_player ) {
        return;
    }

    uint64_t lpEventManager = RPM<uintptr_t> ( local_player + 0x30 );

    if ( !lpEventManager ) {
        return;
    }

    uint64_t lpFxArray = RPM<uintptr_t> ( lpEventManager + 0x31 );

    if ( !lpFxArray ) {
        return;
    }

    const UINT uStunIndex = 5;
    uint64_t lpFxStun = RPM<uintptr_t> ( lpFxArray + ( uStunIndex * sizeof ( PVOID ) ) );

    UINT8 r1 = RPM<UINT8> ( lpFxStun + 0x28 );


    if ( Globals::noflash )
    {
        WPM<uint8_t> ( lpFxStun + 0x40 , 0 );
    }

    else
    {
        WPM<uint8_t> ( lpFxStun + 0x40 , 1 );
    }

}
uint64_t GetCamera ( ) {


    uint64_t GameRenderer1 = RPM<uint64_t> ( base_address + OFFSET_PROFILE );

    if ( !GameRenderer1 )
        return 0;

    uint64_t RenderDeref1 = RPM<uint64_t> ( GameRenderer1 + 0x78 );

    if ( !RenderDeref1 )
        return 0;

    uint64_t EngineLink1 = RPM<uint64_t> ( RenderDeref1 + 0x0 );

    if ( !EngineLink1 )
        return 0;

    uint64_t Engine1 = RPM<uint64_t> ( EngineLink1 + 0x180 );

    return RPM<uint64_t> ( Engine1 + 0x410 );
}

Vector3 GetViewTranslation ( ) {
    return RPM<Vector3> ( GetCamera ( ) + 0x7F0 );
}


Vector3 GetHeadPosition ( uint64_t ent )
{
    return RPM<Vector3> ( ent + 0x0FC0 );
}




constexpr float r2d = 57.2957795131f;
constexpr float d2r = 0.01745329251f;

FORCEINLINE Vector3 CalculateAngle ( Vector3 enemy )
{
    Vector3 dir = enemy - GetViewTranslation ( );

    float x = asin ( dir.z / dir.length ( ) ) * r2d;
    float z = atan ( dir.y / dir.x ) * r2d;

    if ( dir.x >= 0.f ) z += 180.f;
    if ( x > 179.99f ) x -= 360.f;
    else if ( x < -179.99f ) x += 360.f;


    return Vector3 ( x , 0.f , z + 90.f );
}

FORCEINLINE Vector4 CreateFromYawPitchRoll ( float yaw , float pitch , float roll )
{
    auto cosine_yaw = cos ( yaw * 0.5f ); auto sine_yaw = sin ( yaw * 0.5f );
    auto cosine_pitch = cos ( pitch * 0.5f ); auto sine_pitch = sin ( pitch * 0.5f );
    auto cosine_roll = cos ( roll * 0.5f ); auto sine_roll = sin ( roll * 0.5f );

    return Vector4 ( cosine_yaw * cosine_pitch * sine_roll - sine_yaw * sine_pitch * cosine_roll ,
        sine_yaw * cosine_pitch * sine_roll + cosine_yaw * sine_pitch * cosine_roll ,
        sine_yaw * cosine_pitch * cosine_roll - cosine_yaw * sine_pitch * sine_roll ,
        cosine_yaw * cosine_pitch * cosine_roll + sine_yaw * sine_pitch * sine_roll );
}

FORCEINLINE void ClampAngle ( Vector3& angle ) {
    if ( angle.x > 75.f ) angle.x = 75.f;
    else if ( angle.x < -75.f ) angle.x = -75.f;
    if ( angle.z < -180.f ) angle.z += 360.f;
    else if ( angle.z > 180.f ) angle.z -= 360.f;

    angle.y = 0.f;
}


Vector3 W2S ( Vector3 position ) {
    Vector3 ViewTranslation = GetViewTranslation ( );

    uint64_t Camera1 = GetCamera ( );

    // Get View Right
    Vector3 ViewRight1 = RPM<Vector3> ( Camera1 + 0x7C0 );



    // Get View Up
    Vector3 ViewUp1 = RPM<Vector3> ( Camera1 + 0x7D0 );

    // Get View Forward
    Vector3 ViewForward1 = RPM<Vector3> ( Camera1 + 0x7E0 );

    // FOVX
    float FOVX = RPM<float> ( Camera1 + 0x800 );

    // FOVY
    float FOVY = RPM<float> ( Camera1 + 0x814 );


    Vector3 temp = position - ViewTranslation;

    float x = temp.Dot ( ViewRight1 );
    float y = temp.Dot ( ViewUp1 );
    float z = temp.Dot ( ViewForward1 * -1 );


    float width = 1920.f;
    float height = 1080.f;

    return Vector3 ( ( width / 2 ) * ( 1 + x / FOVX / z ) , ( height / 2 ) * ( 1 - y / FOVY / z ) , z );
}

float CrosshairDistance ( Vector3 Entity ) {

    float width = 1920.f;
    float height = 1080.f;
    return sqrt ( pow ( Entity.y - ( height / 2.f ) , 2.f ) + pow ( Entity.x - ( width / 2.f ) , 2.f ) );
}


FORCEINLINE void SetViewAngles ( uintptr_t entity , Vector4 vec )
{


    uint64_t r1 = RPM<uint64_t> ( entity + 0x20 ) + 0xB9A25DD8A6AD943D;

    if ( !r1 ) {
        return;
    }
    uint64_t r2 = RPM<uint64_t> ( r1 + 0x11F0 );
    if ( !r2 )
        return;

    if ( Globals::silentaim ) {

        WPM ( r2 + 0x134 , vec );
    }
    else {
        WPM ( r2 + 0xc0 , vec );

    }
}

DWORD_PTR GetClosetTargetFromCrosshair ( float fov )
{

    DWORD_PTR return_entity = NULL;

    float max_fov = 600.f; // this is aimbot fov

    for ( int i = 0; i <= 11; i++ ) {
        DWORD_PTR pEntity = entitys::EntityObject ( i );
        DWORD_PTR local_player = entitys::local_player ( );


        if ( !pEntity )
            continue;

        auto health = entitys::EntityHealth ( local_player );

        if ( health <= 0 || health > 200 )
            continue;


        //if ( pEntity == local_player  )
            //continue;


        int32_t entityteam = entitys::EntityTeam ( pEntity );
        int32_t localteam = entitys::LocalTeam ( local_player );

        if ( entityteam == localteam )
            continue;



        Vector3 EntityPos = W2S ( GetHeadPosition ( pEntity ) );

        if ( EntityPos.zero ( ) )
            continue;

        float distance = CrosshairDistance ( EntityPos );


        if ( distance <= fov ) {
            fov = distance;
            return_entity = pEntity;
        }
    }

    return return_entity;
}


void silentaim ( ) {


    WPM ( base_address + OFFSET_PSILENT + 0x3 , Globals::silentaim ? 0x134 : 0xc0 );


}


bool NoRecoil1 ( )

{


    uint64_t Weapon = RPM<uint64_t> ( entitys::local_player ( ) + 0x90 );

    if ( !Weapon )
    {
        return false;
    }
    Weapon = RPM<uint64_t> ( Weapon + 0xC8 );

    if ( !Weapon )
    {
        return false;
    }

    uint64_t decryptedweapon = RPM<uint64_t> ( Weapon + 0x290 ) - 0x2B306CB952F73B96;

    if ( !decryptedweapon )
    {
        return false;
    }

    if ( Globals::norecoil )
    {
        WPM<uint8_t> ( decryptedweapon + 0x198 , 0 ); //bye to allow recoil modification

        WPM<float> ( decryptedweapon + 0x18C , Globals::recoilvertvalues ); ///vert
        WPM<float> ( decryptedweapon + 0x17C , Globals::recoilhoriztvalues ); // horizontal 
    }

    else
    {
        WPM<uint8_t> ( decryptedweapon + 0x198 , 1 );

        WPM<float> ( decryptedweapon + 0x18C , 1.f );///vert

        WPM<float> ( decryptedweapon + 0x17C , 1.f );// horizontal 
    }



    return true;

}

bool NoSpread1 ( )

{



    uintptr_t spread = RPM<uintptr_t> ( entitys::local_player ( ) + 0x90 );

    if ( !spread ) {
        return false;
    }

    spread = RPM<uintptr_t> ( spread + 0xC8 );

    if ( !spread ) {
        return false;
    }

    spread = RPM<uintptr_t> ( spread + 0x290 ) + 0xD4CF9346AD08C46A;

    if ( !spread ) {
        return false;
    }

    if ( Globals::nospread )
    {
        WPM<float> ( spread + 0x80 , 0.f );
    }
    else
    {
        WPM<float> ( spread + 0x80 , 0.75f );

    }


    return true;

}



void rundshoot ( )
{
    uint8_t ENABLE_RUN_SHOOT_NOT_ALLOWED_TO_RUN [ ] = { 0x80,0xB9,0x80,0x00 ,0x00 ,0x00, 0x01 }; //80 B9 80 00 00 00 00 //80 B9 80 00 00 00 00
    uint8_t disable_RUN_SHOOT_NOT_ALLOWED_TO_RUN [ ] = { 0x80,0xB9,0x80,0x00 ,0x00 ,0x00, 0x00 }; //80 B9 80 00 00 00 00 //80 B9 80 00 00 00 00

    uint8_t ENABLE_RUN_AND_SHOOT_ALLOWED_TO_RUN [ ] = { 0x80, 0xB9 ,0x80 ,0x00 ,0x00, 0x00 , 0x01 }; // 80 B9 80000000 00 // 80 B9 80 00 00 00 00
    uint8_t disable_RUN_AND_SHOOT_ALLOWED_TO_RUN [ ] = { 0x80, 0xB9 ,0x80 ,0x00 ,0x00, 0x00 , 0x00 }; // 80 B9 80000000 00 // 80 B9 80 00 00 00 00
    if ( Globals::runandshoot ) {
        for ( size_t i = 0; i < 7; i++ ) {
            WPM ( base_address + OFFSET_RUN_SHOOT_NOT_ALLOWED_TO_RUN + i , ENABLE_RUN_SHOOT_NOT_ALLOWED_TO_RUN [ i ] );
        }
        for ( size_t j = 0; j < 7; j++ ) {

            WPM ( base_address + OFFSET_RUN_AND_SHOOT_ALLOWED_TO_RUN + j , ENABLE_RUN_AND_SHOOT_ALLOWED_TO_RUN [ j ] );

        }
    }
    else {
        for ( size_t i = 0; i < 7; i++ ) {
            WPM ( base_address + OFFSET_RUN_SHOOT_NOT_ALLOWED_TO_RUN + i , disable_RUN_SHOOT_NOT_ALLOWED_TO_RUN [ i ] );
        }
        for ( size_t j = 0; j < 7; j++ ) {

            WPM ( base_address + OFFSET_RUN_AND_SHOOT_ALLOWED_TO_RUN + j , disable_RUN_AND_SHOOT_ALLOWED_TO_RUN [ j ] );

        }
    }
}

bool bytenr ( ) {

    uintptr_t Manager = RPM<uintptr_t> ( base_address + NoRecoil_OFFSET );

    if ( !Manager ) {
        return false;
    }

    if ( Globals::deadnorecoil )
    {
        WPM<byte> ( Manager + 0xE34 , 0 );
    }
    else {
        WPM<byte> ( Manager + 0xE34 , 1 );

    }
    return true;

}


void aimbot ( ) {
    while ( GetAsyncKeyState ( 0x01 ) && Globals::aimbot )
    {

        uintptr_t pEntity = GetClosetTargetFromCrosshair ( 4000.f );
        if ( pEntity != NULL )
        {
            Vector3 vPos = GetHeadPosition ( pEntity );
            Vector3 cCalc = CalculateAngle ( vPos );
            ClampAngle ( cCalc );

            Vector4 vQuat = CreateFromYawPitchRoll ( cCalc.z * d2r , 0 , cCalc.x * d2r );


            int Health = entitys::EntityHealth ( entitys::local_player ( ) );

            if ( Health > 20 && Health < 200 )
            {

                SetViewAngles ( entitys::local_player ( ) , vQuat );
            }
        }
        Sleep ( Globals::AimbotSpeed );
    }
}

void MainThread ( )
{
    while ( 1 )
    {

        if ( is_in_game ( ) )
        {
            

            bytenr ( );
            NoSpread1 ( );

            aimbot ( );
            rundshoot ( );
            silentaim ( );
            NoRecoil1 ( );
            CaveriaESP ( );
            noclip ( );
            no_flash ( );
            rapidfire ( );
            speedMod ( );
            noanimations ( );
            chams ( );
            Sleep ( 100 );
        }
    }
}
int main()
{
    SetConsoleTitle(" ");
    pid = GetPID("RainbowSix.exe");
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    base_address = GetModule(hProcess);
    
    unlockall ( );
    auto hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)rungui, 0, 0, 0);
    auto aThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE) MainThread , 0, 0, 0);
    

    while (true)
    {
        

    }
    return 0;
}
