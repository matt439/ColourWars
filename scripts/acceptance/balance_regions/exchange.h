#pragma once

#include "scripts/acceptance/balance_regions/metrics.h"
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace balance_regions
{
    // One team's sampled owner transitions inside one region. Areas use the
    // same exact tile pieces as Measurement, so they reconcile with its cells.
    struct TeamRegionExchange
    {
        double initial_held = 0.0, final_held = 0.0;
        double neutral_claim = 0.0, enemy_capture = 0.0;  // gained by this team
        double lost_to_enemy = 0.0, to_neutral = 0.0;     // lost by this team
        double ally_owner_change = 0.0;                   // same team, new owner
        // Final holding on tiles the enemy held at some sampled step.
        double contested_held = 0.0;
        int enemy_capture_tiles = 0, lost_tiles = 0, first_enemy_capture_step = -1, last_lost_step = -1;
    };
    struct SeatRegionExchange
    {
        double neutral_claim = 0.0, enemy_capture = 0.0, ally_owner_change = 0.0;
        int enemy_capture_tiles = 0;
    };

    // Samples once per step, so a within-step A->B->A flip is invisible.
    // The caller checks that every step keeps the starting tile order, face,
    // source, corners and scored-area bits; the pieces are computed once.
    class ExchangeLedger final
    {
    public:
        static constexpr std::size_t seats = 4;

        ExchangeLedger(std::span<const PaintSurfaceSample> start, const Partition& partition,
            std::array<int, seats> seat_owners) :
            regions_(partition.bands.size() + 1), seat_owners_(seat_owners), team_(regions_), seat_(regions_),
            ever_(start.size(), 0), team_now_(start.size()), owner_now_(start.size())
        {
            pieces_.reserve(start.size() * this->regions_);
            for (std::size_t tile = 0; tile < start.size(); ++tile)
            {
                const std::vector<double> pieces = region_pieces(start[tile], partition);
                this->pieces_.insert(this->pieces_.end(), pieces.begin(), pieces.end());
                this->team_now_[tile] = start[tile].team; this->owner_now_[tile] = start[tile].paint_owner;
                this->ever_[tile] = bit(start[tile].team);
                this->total_ += start[tile].scored_area;
                if (start[tile].team == PlayerTeam::none) { continue; }
                for (std::size_t region = 0; region < this->regions_; ++region)
                { this->team_[region][side(start[tile].team)].initial_held += this->piece(tile, region); }
            }
        }

        void step(int tick, std::span<const PaintSurfaceSample> now)
        {
            need(now.size() == this->team_now_.size(), "Exchange ledger tile count changed");
            for (std::size_t tile = 0; tile < now.size(); ++tile)
            {
                const PlayerTeam before = this->team_now_[tile], after = now[tile].team;
                const int owner = now[tile].paint_owner;
                if (before == after && owner == this->owner_now_[tile]) { continue; }
                // Measurement rejects a neutral tile with an owner.
                need(before != PlayerTeam::none || after != PlayerTeam::none, "Exchange ledger neutral tile changed owner");
                const int seat = this->seat_of(owner);
                for (std::size_t region = 0; region < this->regions_; ++region)
                {
                    const double area = this->piece(tile, region);
                    if (!(area > 0.0)) { continue; }
                    ++this->additions_;
                    if (after == PlayerTeam::none) { this->team_[region][side(before)].to_neutral += area; continue; }
                    TeamRegionExchange& gain = this->team_[region][side(after)];
                    SeatRegionExchange* by = seat >= 0 ? &this->seat_[region][static_cast<std::size_t>(seat)] : nullptr;
                    if (before == PlayerTeam::none)
                    {
                        gain.neutral_claim += area;
                        if (by != nullptr) { by->neutral_claim += area; }
                    }
                    else if (before != after)
                    {
                        TeamRegionExchange& loss = this->team_[region][side(before)];
                        gain.enemy_capture += area; ++gain.enemy_capture_tiles;
                        if (gain.first_enemy_capture_step < 0) { gain.first_enemy_capture_step = tick; }
                        loss.lost_to_enemy += area; ++loss.lost_tiles; loss.last_lost_step = tick;
                        if (by != nullptr) { by->enemy_capture += area; ++by->enemy_capture_tiles; }
                    }
                    else
                    {
                        gain.ally_owner_change += area;
                        if (by != nullptr) { by->ally_owner_change += area; }
                    }
                }
                this->ever_[tile] |= bit(after); this->team_now_[tile] = after; this->owner_now_[tile] = owner;
            }
        }

        void finish(std::span<const PaintSurfaceSample> last)
        {
            need(last.size() == this->team_now_.size(), "Exchange ledger final tile count changed");
            for (std::size_t tile = 0; tile < last.size(); ++tile)
            {
                need(last[tile].team == this->team_now_[tile] && last[tile].paint_owner == this->owner_now_[tile],
                    "Exchange ledger missed the final step");
                if (this->team_now_[tile] == PlayerTeam::none) { continue; }
                const std::size_t team = side(this->team_now_[tile]);
                const std::uint8_t enemy = team == 0 ? 2 : 1;
                for (std::size_t region = 0; region < this->regions_; ++region)
                {
                    this->team_[region][team].final_held += this->piece(tile, region);
                    if ((this->ever_[tile] & enemy) != 0) { this->team_[region][team].contested_held += this->piece(tile, region); }
                }
            }
        }

        // Bookkeeping only: holds by construction. The integrity checks are
        // the reconciliations with Measurement and the arena-wide counters.
        double conservation_residual(std::size_t region, std::size_t team) const
        {
            const TeamRegionExchange& x = this->team_[region][team];
            return x.final_held - x.initial_held -
                (x.neutral_claim + x.enemy_capture - x.lost_to_enemy - x.to_neutral);
        }
        double tolerance() const
        {
            return 8.0 * std::numeric_limits<double>::epsilon() * this->total_ *
                static_cast<double>(this->additions_ + this->ever_.size() + 1);
        }
        // The holding after the last sampled step, by the same bookkeeping.
        double held(std::size_t region, std::size_t team) const
        {
            const TeamRegionExchange& x = this->team_[region][team];
            return x.initial_held + x.neutral_claim + x.enemy_capture - x.lost_to_enemy - x.to_neutral;
        }
        const TeamRegionExchange& team(std::size_t region, std::size_t team) const { return this->team_[region][team]; }
        const SeatRegionExchange& seat(std::size_t region, std::size_t seat) const { return this->seat_[region][seat]; }
        std::size_t regions() const { return this->regions_; }

    private:
        double piece(std::size_t tile, std::size_t region) const { return this->pieces_[tile * this->regions_ + region]; }
        static std::size_t side(PlayerTeam team)
        {
            need(team != PlayerTeam::none, "Exchange ledger booked neutral area to a team");
            return team == PlayerTeam::a ? 0 : 1;
        }
        static std::uint8_t bit(PlayerTeam team)
        { return team == PlayerTeam::a ? 1 : (team == PlayerTeam::b ? 2 : 0); }
        static void need(bool condition, const char* reason) { if (!condition) { throw std::runtime_error(reason); } }
        int seat_of(int owner) const
        {
            for (std::size_t seat = 0; seat < seats; ++seat)
            { if (this->seat_owners_[seat] == owner) { return static_cast<int>(seat); } }
            return -1;
        }

        std::size_t regions_;
        std::array<int, seats> seat_owners_;
        std::vector<double> pieces_;
        std::vector<std::array<TeamRegionExchange, 2>> team_;
        std::vector<std::array<SeatRegionExchange, seats>> seat_;
        std::vector<std::uint8_t> ever_;
        std::vector<PlayerTeam> team_now_;
        std::vector<int> owner_now_;
        double total_ = 0.0;
        std::size_t additions_ = 0;
    };
}
