#include <nano/node/election.hpp>
#include <nano/node/testing.hpp>
#include <nano/test_common/testutil.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST (election, construction)
{
	nano::system system (1);
	nano::genesis genesis;
	auto & node = *system.nodes[0];
	genesis.open->sideband_set (nano::block_sideband (nano::genesis_account, 0, nano::genesis_amount, 1, nano::seconds_since_epoch (), nano::epoch::epoch_0, false, false, false, nano::epoch::epoch_0));
	auto election = node.active.insert (genesis.open).election;
	election->transition_active ();
}

TEST (election, quorum_minimum_flip_success)
{
	nano::system system;
	nano::node_config node_config (nano::get_available_port (), system.logging);
	node_config.online_weight_minimum = nano::genesis_amount;
	node_config.frontiers_confirmation = nano::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	nano::keypair key1;
	nano::block_builder builder;
	auto send1 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (node1.online_reps.delta ())
	             .link (key1.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send1);
	nano::keypair key2;
	auto send2 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (node1.online_reps.delta ())
	             .link (key2.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send2);
	node1.process_active (send1);
	node1.process_active (send2);
	node1.block_processor.flush ();
	auto election{ node1.active.insert (send1) };
	ASSERT_FALSE (election.inserted);
	ASSERT_NE (nullptr, election.election);
	ASSERT_EQ (2, election.election->blocks ().size ());
	auto vote1 (std::make_shared<nano::vote> (nano::dev_genesis_key.pub, nano::dev_genesis_key.prv, std::numeric_limits<uint64_t>::max (), send2));
	ASSERT_EQ (nano::vote_code::vote, node1.active.vote (vote1));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send2->hash ()));
	ASSERT_TRUE (election.election->confirmed ());
}

TEST (election, quorum_minimum_flip_fail)
{
	nano::system system;
	nano::node_config node_config (nano::get_available_port (), system.logging);
	node_config.online_weight_minimum = nano::genesis_amount;
	node_config.frontiers_confirmation = nano::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	nano::keypair key1;
	nano::block_builder builder;
	auto send1 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (node1.online_reps.delta () - 1)
	             .link (key1.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send1);
	nano::keypair key2;
	auto send2 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (node1.online_reps.delta () - 1)
	             .link (key2.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send2);
	node1.process_active (send1);
	node1.process_active (send2);
	node1.block_processor.flush ();
	auto election{ node1.active.insert (send1) };
	ASSERT_FALSE (election.inserted);
	ASSERT_NE (nullptr, election.election);
	ASSERT_EQ (2, election.election->blocks ().size ());
	auto vote1 (std::make_shared<nano::vote> (nano::dev_genesis_key.pub, nano::dev_genesis_key.prv, std::numeric_limits<uint64_t>::max (), send2));
	ASSERT_EQ (nano::vote_code::vote, node1.active.vote (vote1));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
	ASSERT_FALSE (election.election->confirmed ());
}

TEST (election, quorum_minimum_confirm_success)
{
	nano::system system;
	nano::node_config node_config (nano::get_available_port (), system.logging);
	node_config.online_weight_minimum = nano::genesis_amount;
	node_config.frontiers_confirmation = nano::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	nano::keypair key1;
	nano::block_builder builder;
	auto send1 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (node1.online_reps.delta ())
	             .link (key1.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send1);
	node1.process_active (send1);
	node1.block_processor.flush ();
	auto election{ node1.active.insert (send1) };
	ASSERT_FALSE (election.inserted);
	ASSERT_NE (nullptr, election.election);
	ASSERT_EQ (1, election.election->blocks ().size ());
	auto vote1 (std::make_shared<nano::vote> (nano::dev_genesis_key.pub, nano::dev_genesis_key.prv, std::numeric_limits<uint64_t>::max (), send1));
	ASSERT_EQ (nano::vote_code::vote, node1.active.vote (vote1));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
	ASSERT_TRUE (election.election->confirmed ());
}

TEST (election, quorum_minimum_confirm_fail)
{
	nano::system system;
	nano::node_config node_config (nano::get_available_port (), system.logging);
	node_config.online_weight_minimum = nano::genesis_amount;
	node_config.frontiers_confirmation = nano::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	nano::keypair key1;
	nano::block_builder builder;
	auto send1 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (node1.online_reps.delta () - 1)
	             .link (key1.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send1);
	node1.process_active (send1);
	node1.block_processor.flush ();
	auto election{ node1.active.insert (send1) };
	ASSERT_FALSE (election.inserted);
	ASSERT_NE (nullptr, election.election);
	ASSERT_EQ (1, election.election->blocks ().size ());
	auto vote1 (std::make_shared<nano::vote> (nano::dev_genesis_key.pub, nano::dev_genesis_key.prv, std::numeric_limits<uint64_t>::max (), send1));
	ASSERT_EQ (nano::vote_code::vote, node1.active.vote (vote1));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
	ASSERT_FALSE (election.election->confirmed ());
}

namespace nano
{
TEST (election, quorum_minimum_update_weight_before_quorum_checks)
{
	nano::system system;
	nano::node_config node_config (nano::get_available_port (), system.logging);
	node_config.frontiers_confirmation = nano::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	system.wallet (0)->insert_adhoc (nano::dev_genesis_key.prv);
	auto amount = ((nano::uint256_t (node_config.online_weight_minimum.number ()) * nano::online_reps::online_weight_quorum) / 100).convert_to<nano::uint128_t> () - 1;
	nano::keypair key1;
	nano::block_builder builder;
	auto send1 = builder.state ()
	             .account (nano::dev_genesis_key.pub)
	             .previous (nano::genesis_hash)
	             .representative (nano::dev_genesis_key.pub)
	             .balance (amount)
	             .link (key1.pub)
	             .work (0)
	             .sign (nano::dev_genesis_key.prv, nano::dev_genesis_key.pub)
	             .build_shared ();
	node1.work_generate_blocking (*send1);
	auto open1 = builder.state ()
	             .account (key1.pub)
	             .previous (0)
	             .representative (key1.pub)
	             .balance (nano::genesis_amount - amount)
	             .link (send1->hash ())
	             .work (0)
	             .sign (key1.prv, key1.pub)
	             .build_shared ();
	nano::keypair key2;
	auto send2 = builder.state ()
	             .account (key1.pub)
	             .previous (open1->hash ())
	             .representative (key1.pub)
	             .balance (3)
	             .link (key2.pub)
	             .work (0)
	             .sign (key1.prv, key1.pub)
	             .build_shared ();
	node1.work_generate_blocking (*open1);
	node1.work_generate_blocking (*send2);
	node1.process_active (send1);
	node1.block_processor.flush ();
	node1.process (*open1);
	node1.process (*send2);
	node1.block_processor.flush ();
	ASSERT_EQ (node1.ledger.cache.block_count, 4);

	node_config.peering_port = nano::get_available_port ();
	auto & node2 = *system.add_node (node_config);
	node2.process (*send1);
	node2.process (*open1);
	node2.process (*send2);
	system.wallet (1)->insert_adhoc (key1.prv);
	node2.block_processor.flush ();
	ASSERT_EQ (node2.ledger.cache.block_count, 4);

	auto election{ node1.active.insert (send1) };
	ASSERT_FALSE (election.inserted);
	ASSERT_NE (nullptr, election.election);
	ASSERT_EQ (1, election.election->blocks ().size ());
	auto vote1 (std::make_shared<nano::vote> (nano::dev_genesis_key.pub, nano::dev_genesis_key.prv, std::numeric_limits<uint64_t>::max (), send1));
	ASSERT_EQ (nano::vote_code::vote, node1.active.vote (vote1));
	auto vote2 (std::make_shared<nano::vote> (key1.pub, key1.prv, std::numeric_limits<uint64_t>::max (), send1));
	auto channel = node1.network.find_channel (node2.network.endpoint ());
	ASSERT_NE (channel, nullptr);
	ASSERT_TIMELY (10s, !node1.rep_crawler.response (channel, vote2));
	ASSERT_FALSE (election.election->confirmed ());
	{
		nano::lock_guard<nano::mutex> guard (node1.online_reps.mutex);
		// Modify online_m for online_reps to more than is available, this checks that voting below updates it to current online reps.
		node1.online_reps.online_m = node_config.online_weight_minimum.number () + 20;
	}
	ASSERT_EQ (nano::vote_code::vote, node1.active.vote (vote2));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
	ASSERT_TRUE (election.election->confirmed ());
}
}
