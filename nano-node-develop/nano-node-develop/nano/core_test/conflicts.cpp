#include <nano/node/election.hpp>
#include <nano/node/testing.hpp>
#include <nano/test_common/testutil.hpp>

#include <gtest/gtest.h>

#include <boost/variant/get.hpp>

using namespace std::chrono_literals;

TEST (conflicts, start_stop)
{
	nano::system system (1);
	auto & node1 (*system.nodes[0]);
	nano::genesis genesis;
	nano::keypair key1;
	auto send1 (std::make_shared<nano::send_block> (genesis.hash (), key1.pub, 0, nano::dev_genesis_key.prv, nano::dev_genesis_key.pub, 0));
	node1.work_generate_blocking (*send1);
	ASSERT_EQ (nano::process_result::progress, node1.process (*send1).code);
	ASSERT_EQ (0, node1.active.size ());
	auto election1 = node1.active.insert (send1);
	ASSERT_EQ (1, node1.active.size ());
	ASSERT_NE (nullptr, election1.election);
	ASSERT_EQ (1, election1.election->votes ().size ());
}

TEST (conflicts, add_existing)
{
	nano::system system (1);
	auto & node1 (*system.nodes[0]);
	nano::genesis genesis;
	nano::keypair key1;
	auto send1 (std::make_shared<nano::send_block> (genesis.hash (), key1.pub, 0, nano::dev_genesis_key.prv, nano::dev_genesis_key.pub, 0));
	node1.work_generate_blocking (*send1);
	ASSERT_EQ (nano::process_result::progress, node1.process (*send1).code);
	node1.active.insert (send1);
	nano::keypair key2;
	auto send2 (std::make_shared<nano::send_block> (genesis.hash (), key2.pub, 0, nano::dev_genesis_key.prv, nano::dev_genesis_key.pub, 0));
	send2->sideband_set ({});
	auto election1 = node1.active.insert (send2);
	ASSERT_EQ (1, node1.active.size ());
	auto vote1 (std::make_shared<nano::vote> (key2.pub, key2.prv, 0, send2));
	node1.active.vote (vote1);
	ASSERT_NE (nullptr, election1.election);
	ASSERT_EQ (2, election1.election->votes ().size ());
	auto votes (election1.election->votes ());
	ASSERT_NE (votes.end (), votes.find (key2.pub));
}

TEST (conflicts, add_two)
{
	nano::system system (1);
	auto & node1 (*system.nodes[0]);
	nano::genesis genesis;
	nano::keypair key1;
	auto send1 (std::make_shared<nano::send_block> (genesis.hash (), key1.pub, 0, nano::dev_genesis_key.prv, nano::dev_genesis_key.pub, 0));
	node1.work_generate_blocking (*send1);
	ASSERT_EQ (nano::process_result::progress, node1.process (*send1).code);
	node1.active.insert (send1);
	nano::keypair key2;
	auto send2 (std::make_shared<nano::send_block> (send1->hash (), key2.pub, 0, nano::dev_genesis_key.prv, nano::dev_genesis_key.pub, 0));
	node1.work_generate_blocking (*send2);
	ASSERT_EQ (nano::process_result::progress, node1.process (*send2).code);
	node1.active.insert (send2);
	ASSERT_EQ (2, node1.active.size ());
}

TEST (vote_uniquer, null)
{
	nano::block_uniquer block_uniquer;
	nano::vote_uniquer uniquer (block_uniquer);
	ASSERT_EQ (nullptr, uniquer.unique (nullptr));
}

// Show that an identical vote can be uniqued
TEST (vote_uniquer, same_vote)
{
	nano::block_uniquer block_uniquer;
	nano::vote_uniquer uniquer (block_uniquer);
	nano::keypair key;
	auto vote1 (std::make_shared<nano::vote> (key.pub, key.prv, 0, std::make_shared<nano::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0)));
	auto vote2 (std::make_shared<nano::vote> (*vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote2));
}

// Show that a different vote for the same block will have the block uniqued
TEST (vote_uniquer, same_block)
{
	nano::block_uniquer block_uniquer;
	nano::vote_uniquer uniquer (block_uniquer);
	nano::keypair key1;
	nano::keypair key2;
	auto block1 (std::make_shared<nano::state_block> (0, 0, 0, 0, 0, key1.prv, key1.pub, 0));
	auto block2 (std::make_shared<nano::state_block> (*block1));
	auto vote1 (std::make_shared<nano::vote> (key1.pub, key1.prv, 0, block1));
	auto vote2 (std::make_shared<nano::vote> (key1.pub, key1.prv, 0, block2));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote2, uniquer.unique (vote2));
	ASSERT_NE (vote1, vote2);
	ASSERT_EQ (boost::get<std::shared_ptr<nano::block>> (vote1->blocks[0]), boost::get<std::shared_ptr<nano::block>> (vote2->blocks[0]));
}

TEST (vote_uniquer, vbh_one)
{
	nano::block_uniquer block_uniquer;
	nano::vote_uniquer uniquer (block_uniquer);
	nano::keypair key;
	auto block (std::make_shared<nano::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0));
	std::vector<nano::block_hash> hashes;
	hashes.push_back (block->hash ());
	auto vote1 (std::make_shared<nano::vote> (key.pub, key.prv, 0, hashes));
	auto vote2 (std::make_shared<nano::vote> (*vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote2));
}

TEST (vote_uniquer, vbh_two)
{
	nano::block_uniquer block_uniquer;
	nano::vote_uniquer uniquer (block_uniquer);
	nano::keypair key;
	auto block1 (std::make_shared<nano::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0));
	std::vector<nano::block_hash> hashes1;
	hashes1.push_back (block1->hash ());
	auto block2 (std::make_shared<nano::state_block> (1, 0, 0, 0, 0, key.prv, key.pub, 0));
	std::vector<nano::block_hash> hashes2;
	hashes2.push_back (block2->hash ());
	auto vote1 (std::make_shared<nano::vote> (key.pub, key.prv, 0, hashes1));
	auto vote2 (std::make_shared<nano::vote> (key.pub, key.prv, 0, hashes2));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote2, uniquer.unique (vote2));
}

TEST (vote_uniquer, cleanup)
{
	nano::block_uniquer block_uniquer;
	nano::vote_uniquer uniquer (block_uniquer);
	nano::keypair key;
	auto vote1 (std::make_shared<nano::vote> (key.pub, key.prv, 0, std::make_shared<nano::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0)));
	auto vote2 (std::make_shared<nano::vote> (key.pub, key.prv, 1, std::make_shared<nano::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0)));
	auto vote3 (uniquer.unique (vote1));
	auto vote4 (uniquer.unique (vote2));
	vote2.reset ();
	vote4.reset ();
	ASSERT_EQ (2, uniquer.size ());
	auto iterations (0);
	while (uniquer.size () == 2)
	{
		auto vote5 (uniquer.unique (vote1));
		ASSERT_LT (iterations++, 200);
	}
}

TEST (conflicts, reprioritize)
{
	nano::system system (1);
	auto & node1 (*system.nodes[0]);
	nano::genesis genesis;
	nano::keypair key1;
	auto send1 (std::make_shared<nano::send_block> (genesis.hash (), key1.pub, 0, nano::dev_genesis_key.prv, nano::dev_genesis_key.pub, 0));
	node1.work_generate_blocking (*send1);
	auto difficulty1 (send1->difficulty ());
	auto multiplier1 (nano::normalized_multiplier (nano::difficulty::to_multiplier (difficulty1, nano::work_threshold (send1->work_version (), nano::block_details (nano::epoch::epoch_0, false /* unused */, false /* unused */, false /* unused */))), node1.network_params.network.publish_thresholds.epoch_1));
	nano::send_block send1_copy (*send1);
	node1.process_active (send1);
	node1.block_processor.flush ();
	{
		nano::lock_guard<nano::mutex> guard (node1.active.mutex);
		auto existing1 (node1.active.roots.find (send1->qualified_root ()));
		ASSERT_NE (node1.active.roots.end (), existing1);
		ASSERT_EQ (multiplier1, existing1->multiplier);
	}
	node1.work_generate_blocking (send1_copy, difficulty1);
	auto difficulty2 (send1_copy.difficulty ());
	auto multiplier2 (nano::normalized_multiplier (nano::difficulty::to_multiplier (difficulty2, nano::work_threshold (send1_copy.work_version (), nano::block_details (nano::epoch::epoch_0, false /* unused */, false /* unused */, false /* unused */))), node1.network_params.network.publish_thresholds.epoch_1));
	node1.process_active (std::make_shared<nano::send_block> (send1_copy));
	node1.block_processor.flush ();
	{
		nano::lock_guard<nano::mutex> guard (node1.active.mutex);
		auto existing2 (node1.active.roots.find (send1->qualified_root ()));
		ASSERT_NE (node1.active.roots.end (), existing2);
		ASSERT_EQ (multiplier2, existing2->multiplier);
	}
}
