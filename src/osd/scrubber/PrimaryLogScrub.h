// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#pragma once

// the './' includes are marked this way to affect clang-format
#include "./pg_scrubber.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "debug.h"

#include "common/errno.h"
#include "common/scrub_types.h"
#include "messages/MOSDOp.h"
#include "messages/MOSDRepScrub.h"
#include "messages/MOSDRepScrubMap.h"
#include "messages/MOSDScrub.h"
#include "messages/MOSDScrubReserve.h"

#include "osd/OSD.h"
#include "scrub_machine.h"

class PrimaryLogPG;

/**
 * The derivative of PgScrubber that is used by PrimaryLogPG.
 */
class PrimaryLogScrub : public PgScrubber {
 public:
  explicit PrimaryLogScrub(PrimaryLogPG* pg);

  void _scrub_finish() final;

  bool get_store_errors(const scrub_ls_arg_t& arg,
			scrub_ls_result_t& res_inout) const final;

  /**
   *  should we requeue blocked ops?
   *  Yes - if our 'subset_last_applied' is less up-to-date than the
   *  new recovery_state.get_last_update_applied().
   *  (used by PrimaryLogPG::op_applied())
   */
  [[nodiscard]] bool should_requeue_blocked_ops(
    eversion_t last_recovery_applied) const final;

  void stats_of_handled_objects(const object_stat_sum_t& delta_stats,
				const hobject_t& soid) final;

  // the interface used by the scrubber-backend:

  ObjectContextRef get_object_context(const hobject_t& obj) final;
  PrimaryLogPG::OpContextUPtr get_mod_op_context(ObjectContextRef obj_ctx) final;
  void simple_opc_submit(PrimaryLogPG::OpContextUPtr&& op_ctx) final;
  void finish_ctx(PrimaryLogPG::OpContext* op_ctx, int log_op_type) final;
  void add_to_stats(const object_stat_sum_t& stat) final;

 private:
  // we know our PG is actually a PrimaryLogPG. Let's alias the pointer to that object:
  PrimaryLogPG* const m_pl_pg;

  /**
   * Validate consistency of the object info and snap sets.
   */
  //void scrub_snapshot_metadata(ScrubMap& map, const missing_map_t& missing_digest) final;

//  void log_missing(int missing,
//		   const std::optional<hobject_t>& head,
//		   LogChannelRef clog,
//		   const spg_t& pgid,
//		   const char* func,
//		   const char* mode,
//		   bool allow_incomplete_clones);
//
//  int process_clones_to(const std::optional<hobject_t>& head,
//			const std::optional<SnapSet>& snapset,
//			LogChannelRef clog,
//			const spg_t& pgid,
//			const char* mode,
//			bool allow_incomplete_clones,
//			std::optional<snapid_t> target,
//			std::vector<snapid_t>::reverse_iterator* curclone,
//			inconsistent_snapset_wrapper& snap_error);


  // handle our part in stats collection
  object_stat_collection_t m_scrub_cstat;
  void _scrub_clear_state() final;  // which just clears the stats
};
