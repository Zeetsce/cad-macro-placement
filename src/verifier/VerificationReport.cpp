#include "verifier/VerificationReport.hpp"

#include <sstream>

namespace verifier {

std::string VerificationReport::to_string() const {
    std::ostringstream oss;
    oss << "VerificationReport: " << (is_valid ? "VALID" : "INVALID") << "\n";
    oss << "  Overlaps count: " << overlaps.size() << "\n";
    for (const auto& o : overlaps) {
        oss << "    Overlap between block " << o.block_a << " and " << o.block_b
            << ", area: " << o.intersection_area << "\n";
    }
    oss << "  Boundary violations count: " << boundary_escapes.size() << "\n";
    for (const auto& b : boundary_escapes) {
        oss << "    Block " << b.block_id << " escape: overflow_x=" << b.overflow_x
            << ", overflow_y=" << b.overflow_y << "\n";
    }
    oss << "  Grid violations count: " << grid_violations.size() << "\n";
    for (int id : grid_violations) {
        oss << "    Block " << id << " not aligned to grid\n";
    }
    oss << "  Illegal rotations count: " << illegal_rotations.size() << "\n";
    for (int id : illegal_rotations) {
        oss << "    Block " << id << " illegally rotated\n";
    }
    oss << "  Missing or duplicate blocks count: " << missing_or_duplicate_blocks.size() << "\n";
    for (int id : missing_or_duplicate_blocks) {
        oss << "    Block ID issue: " << id << "\n";
    }
    oss << "  HPWL audit: recalculated=" << recalculated_hpwl
        << ", matches=" << (hpwl_matches ? "true" : "false") << "\n";
    return oss.str();
}

} // namespace verifier
