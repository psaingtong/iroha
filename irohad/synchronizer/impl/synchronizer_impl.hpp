/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef IROHA_SYNCHRONIZER_IMPL_HPP
#define IROHA_SYNCHRONIZER_IMPL_HPP

#include "ametsuchi/mutable_factory.hpp"
#include "logger/logger.hpp"
#include "network/block_loader.hpp"
#include "network/consensus_gate.hpp"
#include "synchronizer/synchronizer.hpp"
#include "validation/chain_validator.hpp"

namespace iroha {
  namespace synchronizer {
    class SynchronizerImpl : public Synchronizer {
     public:
      SynchronizerImpl(
          std::shared_ptr<network::ConsensusGate> consensus_gate,
          std::shared_ptr<validation::ChainValidator> validator,
          std::shared_ptr<ametsuchi::MutableFactory> mutableFactory,
          std::shared_ptr<network::BlockLoader> blockLoader);

      ~SynchronizerImpl();

      void process_commit(const shared_model::interface::BlockVariant
                              &committed_block_variant) override;

      rxcpp::observable<Commit> on_commit_chain() override;

     private:
      std::shared_ptr<validation::ChainValidator> validator_;
      std::shared_ptr<ametsuchi::MutableFactory> mutable_factory_;
      std::shared_ptr<network::BlockLoader> block_loader_;

      // internal
      rxcpp::subjects::subject<Commit> notifier_;
      rxcpp::composite_subscription subscription_;

      logger::Logger log_;

      /**
       * Creates a temporary storage out of the provided factory
       * @return pointer to created storage
       */
      std::unique_ptr<ametsuchi::MutableStorage> createTemporaryStorage() const;

      /**
       * Process block, which can be applied to current storage directly:
       *   - apply non-empty block and commit result to Ametsuchi
       *     @or
       *   - don't apply empty block
       * In both cases notify the subscriber about commit
       * @param committed_block_variant to be applied
       */
      void processApplicableBlock(const shared_model::interface::BlockVariant
                                      &committed_block_variant) const;

      /**
       * Process block, which cannot be applied to current storage directly:
       *   - try to download missing blocks from other peers (don't stop, if
       *     they cannot provide blocks at that moment)
       *   - apply the chain on top of existing storage and commit result
       * Committed block variant is not applied, because it's either empty or
       * already exists in downloaded chain
       * @param committed_block_variant to be processed
       * @param storage to apply downloaded chain
       */
      void processUnapplicableBlock(
          const shared_model::interface::BlockVariant &committed_block_variant,
          std::unique_ptr<ametsuchi::MutableStorage> storage) const;
    };
  }  // namespace synchronizer
}  // namespace iroha

#endif  // IROHA_SYNCHRONIZER_IMPL_HPP
