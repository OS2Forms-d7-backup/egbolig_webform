<?php

define('DEFAULT_COUNTRY', 'DANMARK');
define('DEFAULT_EMAIL', 'WEBTEST@bellcom.dk');
define('DEFAULT_PASSWORD', 'retmig');
define('DEFAULT_OFFER_BY_EMAIL', 0);
define('DEFAULT_COMPANY_NO', 99);
define('DEFAULT_COMPANY_NO_PAY_IN', 9);
define('DEFAULT_NO_OF_YEARS', 1);
define('DEFAULT_STATUS', 0);
define('DEFAULT_TENANCY_TYPE', 1);
define('DEFAULT_TYPE_UPDATE', 'I');
define('DEFAULT_TYPE_CREATE', 'A');
define('DEFAULT_MIN_FLOOR', 1);
define('DEFAULT_MAX_FLOOR', 99);
define('DEFAULT_MIN_SQM', 1);
define('DEFAULT_MAX_SQM', 9999);

function egbolig_webform_ws_get_client() {
  $client = &drupal_static(__FUNCTION__);

  if (!isset($client)) {
    $client = new SoapClient(variable_get('soap_webservice'));
  }

  return $client;
}


/**
 * Gets a member object from the webservice based on CPR
 *
 * @return member object, or null if no member is found
 */
function egbolig_webform_ws_get_member_by_cpr($cpr) {
  $client = egbolig_webform_ws_get_client();
  $response = $client->MemberGetListByCprNo(
    array(
      'cprNo' => $cpr,
    )
  );
  $result = $response->MemberGetListByCprNoResult;
  if (!empty((array) $result)) {
    if (isset($result->Member->Cpr42)) {
      $result->Member->Cpr42 = _egbolig_webform_get_number_fixed_length($result->Member->Cpr42, 4);
    }
    if (isset($result->Member->Cpr62)) {
      $result->Member->Cpr62 = _egbolig_webform_get_number_fixed_length($result->Member->Cpr62, 6);
    }

    return $result->Member;
  }
  return NULL;
}

/**
 * Creates a member object via the webservice;
 */
function egbolig_webform_ws_create_member($member) {
  $member->Type = DEFAULT_TYPE_CREATE;

  $client = egbolig_webform_ws_get_client();
  $response = $client->PaymentNewMember(
    [
      'paymentDetails' => [
        'NewAmounts' => [
          'Payment.NewAmount' => [
            'CompanyNo' => DEFAULT_COMPANY_NO,
            'CompanyNoToPayIn' => DEFAULT_COMPANY_NO_PAY_IN,
            'NoOfYears' => DEFAULT_NO_OF_YEARS,
            'Status' => DEFAULT_STATUS,
            'TenancyTypes' => [DEFAULT_TENANCY_TYPE]
          ]
        ]
      ],
      'member' => $member,
    ]
  );
}

/**
 * Updates the member via the webservice.
 * Internally calls _egbolig_ws_update_education
 */
function egbolig_webform_ws_update_member($member) {
  $member->Type = DEFAULT_TYPE_UPDATE;
  $member_array = (array) $member;
  //unsetting the EdgiNo, this sabotages the submission
  unset($member_array['EgdiNo']);

  $client = egbolig_webform_ws_get_client();

  //update member
  $response = $client->MemberUpdate(
    array(
      'member' => $member_array,
    )
  );

  //update criteria
  $response = $client->MemberUpdateCriterias(
    array(
      'member' => $member_array,
    )
  );
  _egbolig_webform_ws_update_education($member_array);
}

/**
 * Updates the children via the webservice.
 * Internally calls ChildDelete to delete the children which are no longed added to membership.
 */
function egbolig_webform_ws_add_children($member, $form_state) {
  $v = $form_state['values']['submitted'];
  $tree = $form_state['webform']['component_tree']['children'];
  $childrenList = _egbolig_webform_get_children_by_member($member->MemberNo, $member->MemberCompanyNo);
  $client = egbolig_webform_ws_get_client();

  for ($i = 1; $i <= 10; $i++) {
    $cid = _egbolig_webform_get_webform_cid($tree, 'child_cpr_nr_' . (string) $i);
    if (isset($v[$cid])) {
      $child_cpr = $v[$cid];

      if (!empty($child_cpr)) {
        if (!egbolig_webform_is_child_exists($child_cpr, $childrenList)) {

          // Attempting to get a child name
          $child_name = "";
          if (module_exists('vcv_serviceplatformen')) {
            $child = vcv_serviceplatformen_get_address($child_cpr);
            if (isset($child['name'])) {
              $child_name = $child['name'];
            }
          }

          $child = array(
            'CompanyNo' => $member->MemberCompanyNo,
            'Cpr4' => substr($child_cpr, -4),
            'Cpr6' => substr($child_cpr, 0, 6),
            'MemberNo' => $member->MemberNo,
            'Name' => $child_name
          );
          $client->ChildAdd(
            array(
              'child' => $child
            )
          );
        }

        egbolig_webform_remove_child_from_list($child_cpr, $childrenList);
      }
    }
  }

  //delete children which are no longer added to the membership
  foreach ($childrenList as $child) {
    $child = array(
      'AutoNo' => $child['AutoNo'],
      'CompanyNo' => $member->MemberCompanyNo,
      'Cpr4' => $child['Cpr4'],
      'Cpr6' => $child['Cpr6'],
      'MemberNo' => $member->MemberNo,
      'Name' => $child['Name']
    );
    $client->ChildDelete(
      array(
        'child' => $child
      )
    );
  }
}

/**
 * Returns a list of educations from webservice
 *
 * @return array of educations
 */
function education_get_list() {
  $client = egbolig_webform_ws_get_client();
  $educations = $client->EducationGetList()->EducationGetListResult->Education;
  $edulist = array();

  foreach ($educations as $education) {
    if (!in_array($education->Id, array(1, 2, 3))) {
      continue;
    }
    elseif (strlen($education->Name) == 1) {
      continue;
    }
    elseif ($education->Name == 'æ' || $education->Name == 'ø' || $education->Name == 'å') {
      continue;
    }
    else {
      $edulist[$education->Id] = mb_convert_case($education->Name, MB_CASE_TITLE, "UTF-8");
    }
  }

  return $edulist;
}

/**
 * Updates the member education via the webservice.
 *
 */
function _egbolig_webform_ws_update_education($member) {
  $client = egbolig_webform_ws_get_client();
  $response = $client->MemberUpdateEducation(
    array(
      'member' => $member,
    )
  );
}

/**
 * Updates the member partner via the webservice.
 * if the partner is not set, it's removed
 *
 */
function _egbolig_webform_ws_update_partner($member, $form_state) {
  $v = $form_state['values']['submitted'];
  $tree = $form_state['webform']['component_tree']['children'];

  //update partner
  if ((int) $v[_egbolig_webform_get_webform_cid($tree, 'has_partner')][0]) {
    $partner_cpr = $v[_egbolig_webform_get_webform_cid($tree, 'parner_cpr_nr')];
    $member->Cpr62 = substr($partner_cpr, 0, 6);
    $member->Cpr42 = substr($partner_cpr, -4);
    $partner_name = "";
    if (module_exists('vcv_serviceplatformen')) {
      $partner = vcv_serviceplatformen_get_address($partner_cpr);
      if (isset($partner['name'])) {
        $partner_name = $partner['name'];
      }
    }
    $client = egbolig_webform_ws_get_client();
    $partner_array = array(
      'memberCompanyNo' => $member->MemberCompanyNo,
      'memberNo' => $member->MemberNo,
      'name' => $partner_name,
      'cpr62' => $member->Cpr62,
      'cpr42' => $member->Cpr42
    );

    $client->MemberUpdatePartner($partner_array);
  } else {
    // remove partner
    $client = egbolig_webform_ws_get_client();
    $partner_array = array(
      'memberCompanyNo' => $member->MemberCompanyNo,
      'memberNo' => $member->MemberNo,
      'name' => '',
    );

    $client->MemberUpdatePartner($partner_array);
  }
}

/**
 * Returns a list of education places from webservice
 *
 * @return array of education places
 */
function education_places() {
  $placeslist = array();
  $client = egbolig_webform_ws_get_client();
  $education_places = $client->EducationalInstitutionGetList()->EducationalInstitutionGetListResult->EducationalInstitution;
  if (count($education_places) > 0) {
    foreach ($education_places as $place) {
      $placeslist[$place->Id] = $place->Name;
    }
  }
  else {
    $placeslist[] = 'Ingen uddannelsessteder';
  }
  return $placeslist;
}

/**
 * Returns a list of accommodations from webservice
 *
 * @return array of accommodations
 */
function accommodation_types() {
  $accommodationlist = array();
  $client = egbolig_webform_ws_get_client();
  $apartment_types = $client->TenancyTypeGetList()->TenancyTypeGetListResult->TenancyType;
  foreach ($apartment_types as $type) {
    $accommodationlist[] = $type->Text;
  }
  return $accommodationlist;
}

/**
 * Returns a list of professions from webservice
 *
 * @return array of professions
 */
function profession_get_list() {
  $professionList = array();
  $client = egbolig_webform_ws_get_client();
  $professions = $client->ProfessionGetList()->ProfessionGetListResult->Profession;
  foreach ($professions as $prof) {
    $professionList[$prof->Id] = $prof->Name;
  }
  return $professionList;
}

/**
 * Returns a list of children from webservice
 *
 * @return array of children
 */
function _egbolig_webform_get_children_by_member($memberNo, $companyNo) {
  $childrenList = array();
  $client = egbolig_webform_ws_get_client();

  $childrenResult = $client->ChildGetByMember(array(
    'companyNo' => $companyNo,
    'memberNo' => $memberNo
  ))->ChildGetByMemberResult;

  if (isset($childrenResult->Child)) {
    $children = $childrenResult->Child;

    if (is_array($children)) {
      foreach ($children as $child) {
        $childrenList[] = array(
          'AutoNo' => $child->AutoNo,
          'Cpr6' => _egbolig_webform_get_number_fixed_length($child->Cpr6, 6),
          'Cpr4' => _egbolig_webform_get_number_fixed_length($child->Cpr4, 4),
          'Name' => $child->Name,
        );
      }
    } else {
      if (isset($children->AutoNo)) {
        $childrenList[] = array(
          'AutoNo' => $children->AutoNo,
          'Cpr6' => _egbolig_webform_get_number_fixed_length($children->Cpr6, 6),
          'Cpr4' => _egbolig_webform_get_number_fixed_length($children->Cpr4, 4),
          'Name' => $children->Name,
        );
      }
    }
  }

  return $childrenList;
}

/**
 * Added wishes
 *
 */
function egbolig_webform_ws_add_wishes($member, $form_state) {
  $v = $form_state['values']['submitted'];
  $tree = $form_state['webform']['component_tree']['children'];
  $client = egbolig_webform_ws_get_client();
  $minRoom = (int) $v[_egbolig_webform_get_webform_cid($tree, 'antal_rum_min')];
  $maxRoom = (int) $v[_egbolig_webform_get_webform_cid($tree, 'antal_rum_max')];
  $wishes = array();

  // Already deleted by calling egbolig_webform_ws_delete_wishes prior to this function
  //delete all wishes before add
  //$response = $client->WishDeleteByMember(array(
  //  'memberNo' => $member->MemberNo,
  //  'memberCompanyNo' => $member->MemberCompanyNo,
  //));

  $submittedRegions = array();
  for ($i = 1; $i <= 4; $i++) {
    // get region IDs
    $regionNo = $v[_egbolig_webform_get_webform_cid($tree, 'priority_' . (string) $i)];
    if (!empty($regionNo)) {

      //skipping duplicates
      if (!in_array($regionNo, $submittedRegions)) {
        $submittedRegions[] = $regionNo;
      }
      else {
        continue;
      }

      $departments_array = array(
        'regionNo' => $regionNo
      );
      $response = $client->DepartmentGetListByRegion($departments_array);
      foreach ($response->DepartmentGetListByRegionResult->Department as $key => $department) {
       for ($j = $minRoom; $j <= $maxRoom; $j++) {
          $wishes[] = array(
            'CompanyNo' => $department->CompanyNo,
            'MemberCompanyNo' => $member->MemberCompanyNo,
            'DepartmentNo' => $department->DepartmentNo,
            'Room' => $j,
            'FloorMax' => DEFAULT_MAX_FLOOR,
            'FloorMin' => DEFAULT_MIN_FLOOR,
            'SqmMax' => DEFAULT_MAX_SQM,
            'SqmMin' => DEFAULT_MIN_SQM,
            'MemberNo' => $member->MemberNo,
            'TenancyType' => DEFAULT_TENANCY_TYPE
          );
        }
      }
    }
  }
  $wish_response = $client->WishAddList(array(
      'wishes' => $wishes
    )
  );
}

/**
 * Call to WishDeleteByMember to delete all member wishes
 *
 * @param $member
 */
function egbolig_webform_ws_delete_wishes($member) {
  $client = egbolig_webform_ws_get_client();

  $client->WishDeleteByMember(
    array(
      'memberCompanyNo' => $member->MemberCompanyNo,
      'memberNo' => $member->MemberNo
    )
  );
}

/**
 * Call to WishGetList
 *
 * @param $member
 */
function egbolig_webform_get_wishes($member) {
  $client = egbolig_webform_ws_get_client();
  $response = $client->WishGetList(
    array(
      'memberCompanyNo' => $member->MemberCompanyNo,
      'memberNo' => $member->MemberNo
    )
  );

  $result = $response->WishGetListResult;
  if (!empty($result)) {
    return $result->Wish;
  }
  return array();
}

/**
 * Call to MembershipActivateMembership, if member is inactive (1 = i bero), it will be activated,
 * if member is already active (0) nothing will be done
 *
 * @param $member
 *
 */
function egbolig_webform_ws_activate_membership($member) {
  $client = egbolig_webform_ws_get_client();

  $response = $client->MembershipGetList(
    array(
      'memberCompanyNo' => $member->MemberCompanyNo,
      'memberNo' => $member->MemberNo
    )
  );

  $result = $response->MembershipGetListResult;
  if ($result) {
    if ($result->Membership->Status == '1') {
      $client->MembershipActivateMembership(array(
          'membership' => array(
            'CompanyNo' => $result->Membership->MemberCompanyNo,
            'MemberCompanyNo' => $member->MemberCompanyNo,
            'MemberNo' => $member->MemberNo,
            'Status' => 0,
            'TenancyType' => DEFAULT_TENANCY_TYPE,
            'WaitListType' => $result->Membership->WaitListType
          )
        )
      );
    }
  }
}