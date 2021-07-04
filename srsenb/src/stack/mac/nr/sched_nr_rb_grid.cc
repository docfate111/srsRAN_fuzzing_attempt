/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsenb/hdr/stack/mac/nr/sched_nr_rb_grid.h"
#include "srsenb/hdr/stack/mac/nr/sched_nr_phy_helpers.h"

namespace srsenb {
namespace sched_nr_impl {

using pusch_grant = sched_nr_interface::pusch_grant;

bwp_slot_grid::bwp_slot_grid(const sched_cell_params& cell_params, uint32_t bwp_id_, uint32_t slot_idx_) :
  dl_rbgs(cell_params.cell_cfg.nof_rbg), ul_rbgs(cell_params.cell_cfg.nof_rbg)
{
  coresets.emplace_back(bwp_id_, slot_idx_, 1, cell_params.cell_cfg.bwps[bwp_id_].rb_width / 6, pdcch_dl_list);
}

void bwp_slot_grid::reset()
{
  for (auto& coreset : coresets) {
    coreset.reset();
  }
  dl_rbgs.reset();
  ul_rbgs.reset();
  pdsch_grants.clear();
  pdcch_dl_list.clear();
  pusch_grants.clear();
  pucch_grants.clear();
}

bwp_res_grid::bwp_res_grid(const sched_cell_params& cell_cfg_, uint32_t bwp_id_) : bwp_id(bwp_id_), cell_cfg(&cell_cfg_)
{
  for (uint32_t sl = 0; sl < SCHED_NR_NOF_SUBFRAMES; ++sl) {
    slots.emplace_back(cell_cfg_, bwp_id, sl);
  }
}

cell_res_grid::cell_res_grid(const sched_cell_params& cell_cfg_) : cell_cfg(&cell_cfg_)
{
  for (uint32_t bwp_id = 0; bwp_id < cell_cfg->cell_cfg.bwps.size(); ++bwp_id) {
    bwps.emplace_back(cell_cfg_, bwp_id);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

slot_bwp_sched::slot_bwp_sched(uint32_t bwp_id, cell_res_grid& phy_grid_) :
  logger(srslog::fetch_basic_logger("MAC")), cfg(*phy_grid_.cell_cfg), bwp_grid(phy_grid_.bwps[bwp_id])
{}

alloc_result slot_bwp_sched::alloc_pdsch(slot_ue& ue, const rbgmask_t& dl_mask)
{
  if (ue.h_dl == nullptr) {
    logger.warning("SCHED: Trying to allocate PDSCH for rnti=0x%x with no available HARQs", ue.rnti);
    return alloc_result::no_rnti_opportunity;
  }
  pdsch_list_t& pdsch_grants = bwp_grid[ue.pdsch_tti].pdsch_grants;
  if (pdsch_grants.full()) {
    logger.warning("SCHED: Maximum number of DL allocations reached");
    return alloc_result::no_grant_space;
  }
  rbgmask_t& pdsch_mask = bwp_grid[ue.pdsch_tti].dl_rbgs;
  if ((pdsch_mask & dl_mask).any()) {
    return alloc_result::sch_collision;
  }
  const uint32_t aggr_idx = 3, coreset_id = 0;
  if (not bwp_grid[ue.pdcch_tti].coresets[coreset_id].alloc_dci(
          pdcch_grant_type_t::dl_data, aggr_idx, coreset_id, &ue)) {
    // Could not find space in PDCCH
    return alloc_result::no_cch_space;
  }

  int mcs = -1, tbs = -1;
  if (ue.h_dl->empty()) {
    mcs      = 20;
    tbs      = 100;
    bool ret = ue.h_dl->new_tx(ue.pdsch_tti, ue.uci_tti, dl_mask, mcs, tbs, 4);
    srsran_assert(ret, "Failed to allocate DL HARQ");
  } else {
    bool ret = ue.h_dl->new_retx(ue.pdsch_tti, ue.uci_tti, dl_mask, &mcs, &tbs);
    srsran_assert(ret, "Failed to allocate DL HARQ retx");
  }

  // Allocation Successful
  pdcch_dl_t& pdcch = bwp_grid[ue.pdcch_tti].pdcch_dl_list.back();
  fill_dci_ue_cfg(ue, pdcch.dci);
  pdsch_grants.emplace_back();
  pdsch_t& grant       = pdsch_grants.back();
  grant.sch.grant.rnti = ue.rnti;
  bitmap_to_prb_array(dl_mask, bwp_grid.nof_prbs(), grant.sch.grant);
  pdsch_mask |= dl_mask;

  return alloc_result::success;
}

alloc_result slot_bwp_sched::alloc_pusch(slot_ue& ue, const rbgmask_t& ul_mask)
{
  if (ue.h_ul == nullptr) {
    logger.warning("SCHED: Trying to allocate PUSCH for rnti=0x%x with no available HARQs", ue.rnti);
    return alloc_result::no_rnti_opportunity;
  }
  pusch_list& pusch_grants = bwp_grid[ue.pusch_tti].pusch_grants;
  if (pusch_grants.full()) {
    logger.warning("SCHED: Maximum number of UL allocations reached");
    return alloc_result::no_grant_space;
  }
  rbgmask_t& pusch_mask = bwp_grid[ue.pusch_tti].ul_rbgs;
  if ((pusch_mask & ul_mask).any()) {
    return alloc_result::sch_collision;
  }
  const uint32_t aggr_idx = 3, coreset_id = 0;
  if (not bwp_grid[ue.pdcch_tti].coresets[coreset_id].alloc_dci(
          pdcch_grant_type_t::ul_data, aggr_idx, coreset_id, &ue)) {
    // Could not find space in PDCCH
    return alloc_result::no_cch_space;
  }

  int mcs = -1, tbs = -1;
  if (ue.h_ul->empty()) {
    mcs      = 20;
    tbs      = 100;
    bool ret = ue.h_ul->new_tx(ue.pusch_tti, ue.pusch_tti, ul_mask, mcs, tbs, ue.cfg->maxharq_tx);
    srsran_assert(ret, "Failed to allocate UL HARQ");
  } else {
    srsran_assert(ue.h_ul->new_retx(ue.pusch_tti, ue.pusch_tti, ul_mask, &mcs, &tbs),
                  "Failed to allocate UL HARQ retx");
  }

  // Allocation Successful
  pdcch_ul_t& pdcch = bwp_grid[ue.pdcch_tti].pdcch_ul_list.back();
  fill_dci_ue_cfg(ue, pdcch.dci);
  pusch_grants.emplace_back();
  pusch_grant& grant = pusch_grants.back();
  grant.dci.ctx.rnti = ue.rnti;
  grant.bitmap       = ul_mask;
  pusch_mask |= ul_mask;

  return alloc_result::success;
}

} // namespace sched_nr_impl
} // namespace srsenb