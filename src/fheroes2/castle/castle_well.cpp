/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string>

#include "agg_image.h"
#include "battle_cell.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "icn.h"
#include "kingdom.h"
#include "resource.h"
#include "speed.h"
#include "text.h"
#include "world.h"

namespace
{
    uint32_t HowManyRecruitMonster( const Castle & castle, Troops & tempArmy, const uint32_t dw, const Funds & add, Funds & res )
    {
        const Monster ms( castle.GetRace(), castle.GetActualDwelling( dw ) );
        if ( !tempArmy.CanJoinTroop( ms ) )
            return 0;

        uint32_t count = castle.getMonstersInDwelling( dw );
        payment_t payment;

        const Kingdom & kingdom = castle.GetKingdom();

        while ( count ) {
            payment = ms.GetCost() * count;
            res = payment;
            payment += add;
            if ( kingdom.AllowPayment( payment ) )
                break;
            --count;
        }

        if ( count > 0 ) {
            tempArmy.JoinTroop( ms, count );
        }

        return count;
    }
}

void Castle::OpenWell( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const fheroes2::ImageRestorer restorer( display, ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2,
                                            ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2, fheroes2::Display::DEFAULT_WIDTH,
                                            fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Point cur_pt( restorer.x(), restorer.y() );
    fheroes2::Point dst_pt( cur_pt.x, cur_pt.y );

    // button exit
    dst_pt.x = cur_pt.x + 578;
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::WELLXTRA, 0, 1 );

    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Button buttonMax( dst_pt.x, dst_pt.y, ICN::BUYMAX, 0, 1 );

    const fheroes2::Rect rectMonster1( cur_pt.x + 20, cur_pt.y + 18, 288, 124 );
    const fheroes2::Rect rectMonster2( cur_pt.x + 20, cur_pt.y + 168, 288, 124 );
    const fheroes2::Rect rectMonster3( cur_pt.x + 20, cur_pt.y + 318, 288, 124 );
    const fheroes2::Rect rectMonster4( cur_pt.x + 334, cur_pt.y + 18, 288, 124 );
    const fheroes2::Rect rectMonster5( cur_pt.x + 334, cur_pt.y + 168, 288, 124 );
    const fheroes2::Rect rectMonster6( cur_pt.x + 334, cur_pt.y + 318, 288, 124 );

    buttonExit.draw();

    std::vector<RandomMonsterAnimation> monsterAnimInfo;
    monsterAnimInfo.emplace_back( Monster( race, DWELLING_MONSTER1 ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER2 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER3 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER4 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER5 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER6 ) ) );

    WellRedrawInfoArea( cur_pt, monsterAnimInfo );

    buttonMax.draw();

    std::vector<u32> alldwellings;
    alldwellings.reserve( 6 );
    alldwellings.push_back( DWELLING_MONSTER6 );
    alldwellings.push_back( DWELLING_MONSTER5 );
    alldwellings.push_back( DWELLING_MONSTER4 );
    alldwellings.push_back( DWELLING_MONSTER3 );
    alldwellings.push_back( DWELLING_MONSTER2 );
    alldwellings.push_back( DWELLING_MONSTER1 );

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow ) {
            break;
        }
        else if ( le.MouseClickLeft( buttonMax.area() ) ) {
            std::vector<Troop> results;
            Funds cur;
            Funds total;
            std::string str;

            const Troops & currentArmy = GetArmy();
            Troops tempArmy( currentArmy );

            for ( const uint32_t dwellingType : alldwellings ) {
                const uint32_t canRecruit = HowManyRecruitMonster( *this, tempArmy, dwellingType, total, cur );
                if ( canRecruit != 0 ) {
                    const Monster ms( race, GetActualDwelling( dwellingType ) );
                    results.emplace_back( ms, canRecruit );
                    total += cur;
                    str.append( ms.GetPluralName( canRecruit ) );
                    str.append( " - " );
                    str.append( std::to_string( canRecruit ) );
                    str.append( "\n" );
                }
            }

            if ( str.empty() ) {
                bool isCreaturePresent = false;
                for ( int i = 0; i < CASTLEMAXMONSTER; ++i ) {
                    if ( dwelling[i] > 0 ) {
                        isCreaturePresent = true;
                        break;
                    }
                }
                if ( isCreaturePresent ) {
                    Dialog::Message( "", _( "Not enough resources to buy monsters." ), Font::BIG, Dialog::OK );
                }
                else {
                    Dialog::Message( "", _( "No monsters available for purchase." ), Font::BIG, Dialog::OK );
                }
            }
            else if ( Dialog::YES == Dialog::ResourceInfo( _( "Buy Monsters" ), str, total, Dialog::YES | Dialog::NO ) ) {
                for ( const Troop & troop : results ) {
                    RecruitMonster( troop, false );
                }
            }
        }
        else if ( ( building & DWELLING_MONSTER1 ) && le.MouseClickLeft( rectMonster1 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, DWELLING_MONSTER1 ), dwelling[0], false ) );
        else if ( ( building & DWELLING_MONSTER2 ) && le.MouseClickLeft( rectMonster2 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER2 ) ), dwelling[1], true ) );
        else if ( ( building & DWELLING_MONSTER3 ) && le.MouseClickLeft( rectMonster3 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER3 ) ), dwelling[2], true ) );
        else if ( ( building & DWELLING_MONSTER4 ) && le.MouseClickLeft( rectMonster4 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER4 ) ), dwelling[3], true ) );
        else if ( ( building & DWELLING_MONSTER5 ) && le.MouseClickLeft( rectMonster5 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER5 ) ), dwelling[4], true ) );
        else if ( ( building & DWELLING_MONSTER6 ) && le.MouseClickLeft( rectMonster6 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER6 ) ), dwelling[5], true ) );

        if ( Game::validateAnimationDelay( Game::CASTLE_UNIT_DELAY ) ) {
            cursor.Hide();
            WellRedrawInfoArea( cur_pt, monsterAnimInfo );

            for ( size_t i = 0; i < monsterAnimInfo.size(); ++i )
                monsterAnimInfo[i].increment();

            buttonMax.draw();
            cursor.Show();
            display.render();
        }
    }
}

void Castle::WellRedrawInfoArea( const fheroes2::Point & cur_pt, const std::vector<RandomMonsterAnimation> & monsterAnimInfo ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::WELLBKG, 0 ), display, cur_pt.x, cur_pt.y );

    Text text;
    fheroes2::Point dst_pt;
    fheroes2::Point pt;

    const fheroes2::Sprite & button = fheroes2::AGG::GetICN( ICN::BUYMAX, 0 );
    const fheroes2::Rect src_rt( 0, 461, button.width(), 19 );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::WELLBKG, 0 ), src_rt.x, src_rt.y, display, cur_pt.x + button.width() + 1, cur_pt.y + 461, src_rt.width, src_rt.height );
    fheroes2::Fill( display, cur_pt.x + button.width(), cur_pt.y + 461, 1, src_rt.height, 0 );

    text.Set( _( "Town Population Information and Statistics" ), Font::BIG );
    dst_pt.x = cur_pt.x + 315 - text.w() / 2;
    dst_pt.y = cur_pt.y + 462;
    text.Blit( dst_pt.x, dst_pt.y );

    u32 dw = DWELLING_MONSTER1;
    size_t monsterId = 0u;

    while ( dw <= DWELLING_MONSTER6 ) {
        bool present = false;
        u32 dw_orig = DWELLING_MONSTER1;
        u32 icnindex = 0;
        u32 available = 0;

        switch ( dw ) {
        case DWELLING_MONSTER1:
            pt.x = cur_pt.x;
            pt.y = cur_pt.y + 1;
            present = ( DWELLING_MONSTER1 & building ) != 0;
            icnindex = 19;
            available = dwelling[0];
            break;
        case DWELLING_MONSTER2:
            pt.x = cur_pt.x;
            pt.y = cur_pt.y + 151;
            present = ( DWELLING_MONSTER2 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER2 );
            icnindex = DWELLING_UPGRADE2 & building ? 25 : 20;
            available = dwelling[1];
            break;
        case DWELLING_MONSTER3:
            pt.x = cur_pt.x;
            pt.y = cur_pt.y + 301;
            present = ( DWELLING_MONSTER3 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER3 );
            icnindex = DWELLING_UPGRADE3 & building ? 26 : 21;
            available = dwelling[2];
            break;
        case DWELLING_MONSTER4:
            pt.x = cur_pt.x + 314;
            pt.y = cur_pt.y + 1;
            present = ( DWELLING_MONSTER4 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER4 );
            icnindex = DWELLING_UPGRADE4 & building ? 27 : 22;
            available = dwelling[3];
            break;
        case DWELLING_MONSTER5:
            pt.x = cur_pt.x + 314;
            pt.y = cur_pt.y + 151;
            present = ( DWELLING_MONSTER5 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER5 );
            icnindex = DWELLING_UPGRADE5 & building ? 28 : 23;
            available = dwelling[4];
            break;
        case DWELLING_MONSTER6:
            pt.x = cur_pt.x + 314;
            pt.y = cur_pt.y + 301;
            present = ( DWELLING_MONSTER6 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER6 );
            icnindex = DWELLING_UPGRADE7 & building ? 30 : ( DWELLING_UPGRADE6 & building ? 29 : 24 );
            available = dwelling[5];
            break;
        default:
            break;
        }

        const Monster monster( race, dw_orig );

        // sprite
        dst_pt.x = pt.x + 21;
        dst_pt.y = pt.y + 35;
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::Get4Building( race ), icnindex ), display, dst_pt.x, dst_pt.y );
        // text
        text.Set( GetStringBuilding( dw_orig, race ), Font::SMALL );
        dst_pt.x = pt.x + 86 - text.w() / 2;
        dst_pt.y = pt.y + 103;
        text.Blit( dst_pt.x, dst_pt.y );

        // name
        text.Set( monster.GetMultiName() );
        dst_pt.x = pt.x + 122 - text.w() / 2;
        dst_pt.y = pt.y + 17;
        text.Blit( dst_pt.x, dst_pt.y );
        // attack
        std::string str;
        str = std::string( _( "Attack" ) ) + ": " + std::to_string( monster.GetAttack() );
        text.Set( str );
        dst_pt.x = pt.x + 268 - text.w() / 2;
        dst_pt.y = pt.y + 22;
        text.Blit( dst_pt.x, dst_pt.y );
        // defense
        str = std::string( _( "Defense" ) ) + ": " + std::to_string( monster.GetDefense() );
        text.Set( str );
        dst_pt.x = pt.x + 268 - text.w() / 2;
        dst_pt.y = pt.y + 34;
        text.Blit( dst_pt.x, dst_pt.y );
        // damage
        str = std::string( _( "Damage" ) ) + ": " + std::to_string( monster.GetDamageMin() ) + "-" + std::to_string( monster.GetDamageMax() );
        text.Set( str );
        dst_pt.x = pt.x + 268 - text.w() / 2;
        dst_pt.y = pt.y + 46;
        text.Blit( dst_pt.x, dst_pt.y );
        // hp
        str = std::string( _( "HP" ) ) + ": " + std::to_string( monster.GetHitPoints() );
        text.Set( str );
        dst_pt.x = pt.x + 268 - text.w() / 2;
        dst_pt.y = pt.y + 58;
        text.Blit( dst_pt.x, dst_pt.y );
        // speed
        str = std::string( _( "Speed" ) ) + ": ";
        text.Set( str );
        dst_pt.x = pt.x + 268 - text.w() / 2;
        dst_pt.y = pt.y + 78;
        text.Blit( dst_pt.x, dst_pt.y );
        text.Set( Speed::String( monster.GetSpeed() ) );
        dst_pt.x = pt.x + 268 - text.w() / 2;
        dst_pt.y = pt.y + 90;
        text.Blit( dst_pt.x, dst_pt.y );

        if ( present ) {
            u32 grown = monster.GetGrown();
            grown += building & BUILD_WELL ? GetGrownWell() : 0;
            if ( DWELLING_MONSTER1 & dw )
                grown += building & BUILD_WEL2 ? GetGrownWel2() : 0;

            text.Set( _( "Growth" ) );
            dst_pt.x = pt.x + 268 - text.w() / 2;
            dst_pt.y = pt.y + 110;
            text.Blit( dst_pt.x, dst_pt.y );
            str = std::string( "+ " ) + std::to_string( grown ) + " / " + _( "week" );
            text.Set( str );
            dst_pt.x = pt.x + 268 - text.w() / 2;
            dst_pt.y = pt.y + 122;
            text.Blit( dst_pt.x, dst_pt.y );

            str = std::string( _( "Available" ) ) + ": ";
            text.Set( str );
            dst_pt.x = pt.x + 44;
            dst_pt.y = pt.y + 122;
            text.Blit( dst_pt.x, dst_pt.y );
            text.Set( std::to_string( available ), Font::YELLOW_BIG );
            dst_pt.x = pt.x + 129 - text.w() / 2;
            dst_pt.y = pt.y + 119;
            text.Blit( dst_pt.x, dst_pt.y );
        }

        // monster
        const bool flipMonsterSprite = ( dw >= DWELLING_MONSTER4 );

        const fheroes2::Sprite & smonster = fheroes2::AGG::GetICN( monsterAnimInfo[monsterId].icnFile(), monsterAnimInfo[monsterId].frameId() );
        if ( flipMonsterSprite )
            dst_pt.x = pt.x + 193 - ( smonster.x() + smonster.width() ) + ( monster.isWide() ? CELLW / 2 : 0 ) + monsterAnimInfo[monsterId].offset();
        else
            dst_pt.x = pt.x + 193 + smonster.x() - ( monster.isWide() ? CELLW / 2 : 0 ) - monsterAnimInfo[monsterId].offset();

        dst_pt.y = pt.y + 124 + smonster.y();

        fheroes2::Point inPos( 0, 0 );
        fheroes2::Point outPos( dst_pt.x, dst_pt.y );
        fheroes2::Size inSize( smonster.width(), smonster.height() );

        if ( fheroes2::FitToRoi( smonster, inPos, display, outPos, inSize,
                                 fheroes2::Rect( cur_pt.x, cur_pt.y, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) ) ) {
            fheroes2::Blit( smonster, inPos, display, outPos, inSize, flipMonsterSprite );
        }

        dw <<= 1;
        ++monsterId;
    }
}
