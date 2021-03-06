/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_SHARED_MODEL_QUERY_VALIDATOR_HPP
#define IROHA_SHARED_MODEL_QUERY_VALIDATOR_HPP

#include <boost/variant/static_visitor.hpp>

#include "backend/protobuf/queries/proto_query.hpp"
#include "validators/answer.hpp"

namespace shared_model {
  namespace validation {

    /**
     * Visitor used by query validator to validate each concrete query
     * @tparam FieldValidator - field validator type
     */
    template <typename FieldValidator>
    class QueryValidatorVisitor
        : public boost::static_visitor<ReasonsGroupType> {
     public:
      QueryValidatorVisitor(const FieldValidator &validator = FieldValidator())
          : validator_(validator) {}

      ReasonsGroupType operator()(const interface::GetAccount &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetAccount";

        validator_.validateAccountId(reason, qry.accountId());

        return reason;
      }

      ReasonsGroupType operator()(const interface::GetSignatories &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetSignatories";

        validator_.validateAccountId(reason, qry.accountId());

        return reason;
      }

      ReasonsGroupType operator()(
          const interface::GetAccountTransactions &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetAccountTransactions";

        validator_.validateAccountId(reason, qry.accountId());

        return reason;
      }

      ReasonsGroupType operator()(
          const interface::GetAccountAssetTransactions &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetAccountAssetTransactions";

        validator_.validateAccountId(reason, qry.accountId());
        validator_.validateAssetId(reason, qry.assetId());

        return reason;
      }

      ReasonsGroupType operator()(const interface::GetTransactions &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetTransactions";

        const auto &hashes = qry.transactionHashes();
        if (hashes.size() == 0) {
          reason.second.push_back("tx_hashes cannot be empty");
        }

        for (const auto &h : hashes) {
          validator_.validateHash(reason, h);
        }

        return reason;
      }

      ReasonsGroupType operator()(
          const interface::GetAccountAssets &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetAccountAssets";

        validator_.validateAccountId(reason, qry.accountId());
        return reason;
      }

      ReasonsGroupType operator()(
          const interface::GetAccountDetail &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetAccountDetail";

        validator_.validateAccountId(reason, qry.accountId());

        return reason;
      }

      ReasonsGroupType operator()(const interface::GetRoles &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetRoles";

        return reason;
      }

      ReasonsGroupType operator()(
          const interface::GetRolePermissions &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetRolePermissions";

        validator_.validateRoleId(reason, qry.roleId());

        return reason;
      }

      ReasonsGroupType operator()(const interface::GetAssetInfo &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetAssetInfo";

        validator_.validateAssetId(reason, qry.assetId());

        return reason;
      }

      ReasonsGroupType operator()(
          const interface::GetPendingTransactions &qry) const {
        ReasonsGroupType reason;
        reason.first = "GetPendingTransactions";

        return reason;
      }

     private:
      FieldValidator validator_;
    };

    /**
     * Class that validates query field from query
     * @tparam FieldValidator - field validator type
     * @tparam QueryFieldValidator - concrete query validator type
     */
    template <typename FieldValidator, typename QueryFieldValidator>
    class QueryValidator {
     public:
      QueryValidator(const FieldValidator &field_validator = FieldValidator(),
                     const QueryFieldValidator &query_field_validator =
                         QueryFieldValidator())
          : field_validator_(field_validator),
            query_field_validator_(query_field_validator) {}

      /**
       * Applies validation to given query
       * @param qry - query to validate
       * @return Answer containing found error if any
       */
      Answer validate(const interface::Query &qry) const {
        Answer answer;
        std::string qry_reason_name = "Query";
        ReasonsGroupType qry_reason(qry_reason_name, GroupedReasons());

        field_validator_.validateCreatorAccountId(qry_reason,
                                                  qry.creatorAccountId());
        field_validator_.validateCreatedTime(qry_reason, qry.createdTime());
        field_validator_.validateCounter(qry_reason, qry.queryCounter());

        if (not qry_reason.second.empty()) {
          answer.addReason(std::move(qry_reason));
        }

        auto qry_case = static_cast<const shared_model::proto::Query &>(qry)
                            .getTransport()
                            .payload()
                            .query_case();
        if (iroha::protocol::Query_Payload::QUERY_NOT_SET == qry_case) {
          ReasonsGroupType reason;
          reason.first = "Undefined";
          reason.second.push_back("query is undefined");
          answer.addReason(std::move(reason));
        } else {
          auto reason = boost::apply_visitor(query_field_validator_, qry.get());
          if (not reason.second.empty()) {
            answer.addReason(std::move(reason));
          }
        }

        return answer;
      }

     protected:
      Answer answer_;
      FieldValidator field_validator_;
      QueryFieldValidator query_field_validator_;
    };
  }  // namespace validation
}  // namespace shared_model

#endif  // IROHA_SHARED_MODEL_QUERY_VALIDATOR_HPP
