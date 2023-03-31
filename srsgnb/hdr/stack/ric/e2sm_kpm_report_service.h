/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 *
 */

#include "srsgnb/hdr/stack/ric/e2sm_kpm.h"
#include "srsgnb/hdr/stack/ric/e2sm_kpm_common.h"
#include "srsran/asn1/e2ap.h"
#include "srsran/asn1/e2sm.h"
#include "srsran/asn1/e2sm_kpm_v2.h"
#include "srsran/common/timers.h"
#include "srsran/srsran.h"

#ifndef SRSRAN_E2SM_KPM_ACTION_DATA_H
#define SRSRAN_E2SM_KPM_ACTION_DATA_H

using namespace asn1::e2ap;
using namespace asn1::e2sm_kpm;

class e2sm_kpm_report_service
{
public:
  e2sm_kpm_report_service() = delete;
  e2sm_kpm_report_service(e2sm_kpm* e2sm_kpm, uint16_t action_id, e2_sm_kpm_action_definition_s action_definition);
  virtual ~e2sm_kpm_report_service() = default;

  virtual bool _initialize_ric_ind_hdr();
  virtual bool _initialize_ric_ind_msg() = 0;
  virtual bool collect_meas_data()       = 0;
  virtual bool is_ric_ind_ready()        = 0;
  virtual bool clear_collected_data()    = 0;

  virtual bool _start_meas_collection();
  bool         stop();
  virtual bool _stop_meas_collection();
  virtual bool _reschedule_meas_collection();

  std::vector<e2sm_kpm_label_enum> _get_present_labels(const meas_info_item_s& action_meas_info_item);
  meas_record_item_c::types
  _get_meas_data_type(std::string meas_name, e2sm_kpm_label_enum label, meas_record_l& meas_record_list);

  e2_sm_kpm_ind_hdr_s& get_ind_hdr() { return ric_ind_header_generic; };
  e2_sm_kpm_ind_msg_s& get_ind_msg() { return ric_ind_message_generic; };

  e2sm_kpm*                                      parent;
  uint16_t                                       action_id;
  e2_sm_kpm_action_definition_s                  action_def_generic;
  e2_sm_kpm_ind_msg_s::ind_msg_formats_c_::types ind_msg_format;
  e2_sm_kpm_ind_hdr_s                            ric_ind_header_generic;
  e2_sm_kpm_ind_msg_s                            ric_ind_message_generic;

  bool  cell_global_id_present = false;
  cgi_c cell_global_id;

  // hdr format 1 in base class, as all types use it
  e2_sm_kpm_ind_hdr_format1_s& ric_ind_header;

  uint32_t             granul_period = 0;
  srsran::unique_timer meas_collection_timer; // for measurements collection
};

class e2sm_kpm_report_service_style1 : public e2sm_kpm_report_service
{
public:
  e2sm_kpm_report_service_style1(e2sm_kpm*                     e2sm_kpm,
                                 uint16_t                      action_id,
                                 e2_sm_kpm_action_definition_s action_definition);
  virtual ~e2sm_kpm_report_service_style1() = default;

  static bool process_ric_action_definition(e2sm_kpm* e2sm_kpm, e2_sm_kpm_action_definition_s& action_definition);

  virtual bool _initialize_ric_ind_msg();
  virtual bool collect_meas_data();
  virtual bool is_ric_ind_ready();
  virtual bool clear_collected_data();

private:
  meas_data_item_s&
  _get_meas_data_item(std::string meas_name, e2sm_kpm_label_enum label, uint32_t ue_id, bool& ref_found);

  e2_sm_kpm_action_definition_format1_s& action_def;
  e2_sm_kpm_ind_msg_format1_s&           ric_ind_message;
};

class e2sm_kpm_report_service_style2 : public e2sm_kpm_report_service
{
public:
  e2sm_kpm_report_service_style2(e2sm_kpm*                     e2sm_kpm,
                                 uint16_t                      action_id,
                                 e2_sm_kpm_action_definition_s action_definition);
  virtual ~e2sm_kpm_report_service_style2() = default;

  static bool process_ric_action_definition(e2sm_kpm* e2sm_kpm, e2_sm_kpm_action_definition_s& action_definition);

  virtual bool _initialize_ric_ind_msg();
  virtual bool collect_meas_data();
  virtual bool is_ric_ind_ready();
  virtual bool clear_collected_data();

private:
  e2_sm_kpm_action_definition_format2_s& action_def;
  e2_sm_kpm_ind_msg_format1_s&           ric_ind_message;
};

class e2sm_kpm_report_service_style3 : public e2sm_kpm_report_service
{
public:
  e2sm_kpm_report_service_style3(e2sm_kpm*                     e2sm_kpm,
                                 uint16_t                      action_id,
                                 e2_sm_kpm_action_definition_s action_definition);
  virtual ~e2sm_kpm_report_service_style3() = default;

  static bool process_ric_action_definition(e2sm_kpm* e2sm_kpm, e2_sm_kpm_action_definition_s& action_definition);

  virtual bool _initialize_ric_ind_msg();
  virtual bool collect_meas_data();
  virtual bool is_ric_ind_ready();
  virtual bool clear_collected_data();

private:
  e2_sm_kpm_action_definition_format3_s& action_def;
  e2_sm_kpm_ind_msg_format2_s&           ric_ind_message;
};

class e2sm_kpm_report_service_style4 : public e2sm_kpm_report_service
{
public:
  e2sm_kpm_report_service_style4(e2sm_kpm*                     e2sm_kpm,
                                 uint16_t                      action_id,
                                 e2_sm_kpm_action_definition_s action_definition);
  virtual ~e2sm_kpm_report_service_style4() = default;

  static bool process_ric_action_definition(e2sm_kpm* e2sm_kpm, e2_sm_kpm_action_definition_s& action_definition);

  virtual bool _initialize_ric_ind_msg();
  virtual bool collect_meas_data();
  virtual bool is_ric_ind_ready();
  virtual bool clear_collected_data();

private:
  e2_sm_kpm_action_definition_format4_s& action_def;
  e2_sm_kpm_ind_msg_format3_s&           ric_ind_message;
};

class e2sm_kpm_report_service_style5 : public e2sm_kpm_report_service
{
public:
  e2sm_kpm_report_service_style5(e2sm_kpm*                     e2sm_kpm,
                                 uint16_t                      action_id,
                                 e2_sm_kpm_action_definition_s action_definition);
  virtual ~e2sm_kpm_report_service_style5() = default;

  static bool process_ric_action_definition(e2sm_kpm* e2sm_kpm, e2_sm_kpm_action_definition_s& action_definition);

  virtual bool _initialize_ric_ind_msg();
  virtual bool collect_meas_data();
  virtual bool is_ric_ind_ready();
  virtual bool clear_collected_data();

private:
  e2_sm_kpm_action_definition_format5_s& action_def;
  e2_sm_kpm_ind_msg_format3_s&           ric_ind_message;
};

#endif // SRSRAN_E2SM_KPM_ACTION_DATA_H
