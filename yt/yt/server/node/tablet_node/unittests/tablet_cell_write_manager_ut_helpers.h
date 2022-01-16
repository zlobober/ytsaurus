#pragma once

#include <yt/yt/core/test_framework/framework.h>

#include "sorted_store_manager_ut_helpers.h"
#include "ordered_dynamic_store_ut_helpers.h"
#include "simple_transaction_supervisor.h"
#include "simple_tablet_manager.h"

#include <yt/yt/server/node/tablet_node/automaton.h>
#include <yt/yt/server/node/tablet_node/bootstrap.h>
#include <yt/yt/server/node/tablet_node/tablet.h>
#include <yt/yt/server/node/tablet_node/tablet_slot.h>
#include <yt/yt/server/node/tablet_node/transaction.h>
#include <yt/yt/server/node/tablet_node/transaction_manager.h>
#include <yt/yt/server/node/tablet_node/serialize.h>

#include <yt/yt/server/lib/tablet_node/config.h>

#include <yt/yt/server/lib/hive/transaction_lease_tracker.h>

#include <yt/yt/server/lib/hydra_common/mock/simple_hydra_manager_mock.h>

#include <yt/yt/ytlib/tablet_client/config.h>

#include <yt/yt/ytlib/transaction_client/helpers.h>

#include <yt/yt/client/transaction_client/helpers.h>

#include <yt/yt/client/security_client/public.h>

#include <yt/yt/client/table_client/helpers.h>

namespace NYT::NTabletNode {
namespace {

// TODO(max42): split into .cpp and .h.

using namespace NConcurrency;
using namespace NTableClient;
using namespace NTabletClient;
using namespace NSecurityClient;
using namespace NTransactionClient;
using namespace NHydra;
using namespace NHiveServer;
using namespace NRpc;
using namespace NLogging;

////////////////////////////////////////////////////////////////////////////////

static const TLogger Logger("Test");

////////////////////////////////////////////////////////////////////////////////

class TSimpleTabletSlot
    : public ITransactionManagerHost
{
public:
    static constexpr TCellId CellId = {0, 42};
    static constexpr TCellTag CellTag = 42;

    explicit TSimpleTabletSlot(TTabletOptions options)
    {
        AutomatonQueue_ = New<TActionQueue>("Automaton");
        AutomatonInvoker_ = AutomatonQueue_->GetInvoker();
        Automaton_ = New<TTabletAutomaton>(/*asyncSnapshotInvoker*/ AutomatonInvoker_, CellId);
        HydraManager_ = New<TSimpleHydraManagerMock>(Automaton_, AutomatonInvoker_, NTabletNode::GetCurrentReign());
        TransactionManager_ = New<TTransactionManager>(New<TTransactionManagerConfig>(), /*transactionManagerHost*/ this, CreateNullTransactionLeaseTracker());
        TransactionSupervisor_ = New<TSimpleTransactionSupervisor>(TransactionManager_, HydraManager_, Automaton_, AutomatonInvoker_);
        TabletManager_ = New<TSimpleTabletManager>(TransactionManager_, HydraManager_, Automaton_, AutomatonInvoker_);
        TabletCellWriteManager_ = CreateTabletCellWriteManager(TabletManager_, HydraManager_, Automaton_, TMemoryUsageTrackerGuard(), AutomatonInvoker_);

        TabletManager_->InitializeTablet(options);
        TabletCellWriteManager_->Initialize();
    }

    NHydra::ISimpleHydraManagerPtr GetSimpleHydraManager() override
    {
        return HydraManager_;
    }

    const NHydra::TCompositeAutomatonPtr& GetAutomaton() override
    {
        return Automaton_;
    }

    IInvokerPtr GetAutomatonInvoker(EAutomatonThreadQueue /*queue*/ = EAutomatonThreadQueue::Default) override
    {
        return AutomatonInvoker_;
    }

    IInvokerPtr GetEpochAutomatonInvoker(EAutomatonThreadQueue /*queue*/ = EAutomatonThreadQueue::Default) override
    {
        return AutomatonInvoker_;
    }

    IInvokerPtr GetGuardedAutomatonInvoker(EAutomatonThreadQueue /*queue*/ = EAutomatonThreadQueue::Default) override
    {
        return AutomatonInvoker_;
    }

    void Shutdown()
    {
        YT_VERIFY(HydraManager_->GetPendingMutationCount() == 0);
        AutomatonQueue_->Shutdown(/*graceful*/ true);
        AutomatonQueue_.Reset();
        AutomatonInvoker_.Reset();
        Automaton_.Reset();
        HydraManager_.Reset();
        TransactionManager_.Reset();
        TransactionSupervisor_.Reset();
        TabletManager_.Reset();
        TabletCellWriteManager_.Reset();
    }

    const NHiveServer::ITransactionSupervisorPtr& GetTransactionSupervisor() override
    {
        // Lease checking is disabled, so transaction supervisor is not needed.
        YT_UNIMPLEMENTED();
    }

    const TRuntimeTabletCellDataPtr& GetRuntimeData() override
    {
        static TRuntimeTabletCellDataPtr RuntimeTabletCellData = nullptr;
        return RuntimeTabletCellData;
    }

    NTransactionClient::TTimestamp GetLatestTimestamp() override
    {
        return LatestTimestamp_;
    }

    NObjectClient::TCellTag GetNativeCellTag() override
    {
        return TCellTag();
    }

    NHydra::TCellId GetCellId() override
    {
        return CellId;
    }

    void SetLatestTimestamp(TTimestamp timestamp)
    {
        LatestTimestamp_ = timestamp;
    }

    const TSimpleTabletManagerPtr& TabletManager()
    {
        return TabletManager_;
    }

    const ITabletCellWriteManagerPtr& TabletCellWriteManager()
    {
        return TabletCellWriteManager_;
    }

    const TSimpleHydraManagerMockPtr& HydraManager()
    {
        return HydraManager_;
    }

    const TTransactionManagerPtr& TransactionManager()
    {
        return TransactionManager_;
    }

    const TSimpleTransactionSupervisorPtr& TransactionSupervisor()
    {
        return TransactionSupervisor_;
    }

private:
    TSimpleHydraManagerMockPtr HydraManager_;
    TActionQueuePtr AutomatonQueue_;
    IInvokerPtr AutomatonInvoker_;
    TCompositeAutomatonPtr Automaton_;
    TTransactionManagerPtr TransactionManager_;
    TSimpleTransactionSupervisorPtr TransactionSupervisor_;
    TSimpleTabletManagerPtr TabletManager_;
    ITabletCellWriteManagerPtr TabletCellWriteManager_;

    TTimestamp LatestTimestamp_ = 4242;
};

DECLARE_REFCOUNTED_CLASS(TSimpleTabletSlot)
DEFINE_REFCOUNTED_TYPE(TSimpleTabletSlot)

////////////////////////////////////////////////////////////////////////////////

class TTabletCellWriteManagerTestBase
    : public testing::Test
{
protected:
    TSimpleTabletSlotPtr TabletSlot_;

    virtual TTabletOptions GetOptions() const = 0;

    void SetUp() override
    {
        TabletSlot_ = New<TSimpleTabletSlot>(GetOptions());
    }

    void TearDown() override
    {
        TabletSlot_->Shutdown();
    }

    const ITabletCellWriteManagerPtr& TabletCellWriteManager()
    {
        return TabletSlot_->TabletCellWriteManager();
    }

    IInvokerPtr AutomatonInvoker()
    {
        return TabletSlot_->GetAutomatonInvoker();
    }

    TSimpleHydraManagerMockPtr HydraManager()
    {
        return TabletSlot_->HydraManager();
    }

    TTransactionManagerPtr TransactionManager()
    {
        return TabletSlot_->TransactionManager();
    }

    TSimpleTransactionSupervisorPtr TransactionSupervisor()
    {
        return TabletSlot_->TransactionSupervisor();
    }

    TTabletId MakeTabletTransactionId(TTimestamp timestamp, int hash = 0, EAtomicity atomicity = EAtomicity::Full)
    {
        return NTransactionClient::MakeTabletTransactionId(atomicity, TSimpleTabletSlot::CellTag, timestamp, hash);
    }

    // Recall that this method may wait on blocked row.

    TFuture<void> WriteUnversionedRows(
        TTransactionId transactionId,
        std::vector<TUnversionedOwningRow> rows,
        TTransactionSignature signature = -1,
        TTransactionGeneration generation = 0)
    {
        auto* tablet = TabletSlot_->TabletManager()->GetTablet();
        auto tabletSnapshot = tablet->BuildSnapshot(nullptr);
        return BIND([
            transactionId,
            rows = std::move(rows),
            signature,
            generation,
            tabletWriteManager = TabletCellWriteManager(),
            tabletSnapshot
        ] {
            TWireProtocolWriter writer;
            i64 dataWeight = 0;
            for (const auto& row : rows) {
                writer.WriteCommand(EWireProtocolCommand::WriteRow);
                writer.WriteUnversionedRow(row);
                dataWeight += GetDataWeight(row);
            }
            auto wireData = writer.Finish();
            struct TTag {};
            TWireProtocolReader reader(MergeRefsToRef<TTag>(wireData));
            TFuture<void> future;
            tabletWriteManager->Write(
                tabletSnapshot,
                transactionId,
                TimestampFromTransactionId(transactionId),
                TDuration::Max(),
                signature,
                generation,
                rows.size(),
                dataWeight,
                /*versioned*/ false,
                TSyncReplicaIdList(),
                &reader,
                &future);

            // NB: we are not going to return future since it will be set only when
            // WriteRows mutation (or mutations) are applied; we are applying mutations
            // manually in these unittests, so this future is meaningless.
            // Still, it is useful to check that no error is thrown in WriteRows mutation handler.
            future
                .Subscribe(BIND([] (const TError& error) {
                    YT_VERIFY(error.IsOK());
                }));
        })
            .AsyncVia(AutomatonInvoker())
            .Run();
    }

    TFuture<void> WriteVersionedRows(
        TTransactionId transactionId,
        std::vector<TVersionedOwningRow> rows,
        TTransactionSignature signature = -1)
    {
        auto* tablet = TabletSlot_->TabletManager()->GetTablet();
        auto tabletSnapshot = tablet->BuildSnapshot(nullptr);
        auto asyncResult = BIND([transactionId, rows = std::move(rows), signature, tabletWriteManager = TabletCellWriteManager(), tabletSnapshot] {
            TWireProtocolWriter writer;
            i64 dataWeight = 0;
            for (const auto& row : rows) {
                writer.WriteCommand(EWireProtocolCommand::VersionedWriteRow);
                writer.WriteVersionedRow(row);
                dataWeight += GetDataWeight(row);
            }
            auto wireData = writer.Finish();
            struct TTag {};
            TWireProtocolReader reader(MergeRefsToRef<TTag>(wireData));
            TFuture<void> asyncResult;

            TAuthenticationIdentity identity(ReplicatorUserName);
            TCurrentAuthenticationIdentityGuard guard(&identity);

            tabletWriteManager->Write(
                tabletSnapshot,
                transactionId,
                TimestampFromTransactionId(transactionId),
                TDuration::Max(),
                signature,
                /*generation*/ 0,
                rows.size(),
                dataWeight,
                /*versioned*/ true,
                TSyncReplicaIdList(),
                &reader,
                &asyncResult);

            // NB: we are not going to return asyncResult since it will be set only when
            // WriteRows mutation (or mutations) are applied; we are applying mutations
            // manually in these unittests, so this future is meaningless.
            // Still, it is useful to check that no error is thrown in WriteRows mutation handler.
            asyncResult
                .Subscribe(BIND([] (TError error) {
                    YT_VERIFY(error.IsOK());
                }));
        })
            .AsyncVia(AutomatonInvoker())
            .Run();
        return asyncResult;
    }

    auto RunInAutomaton(auto callable)
    {
        auto result = WaitFor(
            BIND(callable)
                .AsyncVia(AutomatonInvoker())
                .Run());
        if constexpr (!std::is_same_v<std::decay_t<decltype(result)>, TError>) {
            return result
                .ValueOrThrow();
        } else {
            result
                .ThrowOnError();
            return;
        }
    }

    TFuture<void> PrepareTransactionCommit(TTransactionId transactionId, bool persistent, TTimestamp prepareTimestamp)
    {
        return TransactionSupervisor()->PrepareTransactionCommit(
            transactionId,
            persistent,
            prepareTimestamp);
    }

    TFuture<void> CommitTransaction(TTransactionId transactionId, TTimestamp commitTimestamp)
    {
        return TransactionSupervisor()->CommitTransaction(
            transactionId,
            commitTimestamp);
    }

    TFuture<void> PrepareAndCommitTransaction(TTransactionId transactionId, bool persistent, TTimestamp prepareAndCommitTimestamp)
    {
        auto asyncPrepare = PrepareTransactionCommit(transactionId, persistent, prepareAndCommitTimestamp);
        auto asyncCommit = CommitTransaction(transactionId, prepareAndCommitTimestamp);
        return AllSucceeded<void>({asyncPrepare, asyncCommit});
    }

    void ExpectFullyUnlocked()
    {
        auto* tablet = TabletSlot_->TabletManager()->GetTablet();

        auto [lockCount, hasActiveLocks] = RunInAutomaton([&] {
            return std::make_pair(tablet->GetTabletLockCount(), tablet->GetStoreManager()->HasActiveLocks());
        });

        EXPECT_EQ(0, lockCount);
        EXPECT_FALSE(hasActiveLocks);
    }

    bool HasActiveStoreLocks()
    {
        auto* tablet = TabletSlot_->TabletManager()->GetTablet();

        return RunInAutomaton([&] {
            return tablet->GetStoreManager()->HasActiveLocks();
        });
    }
};

////////////////////////////////////////////////////////////////////////////////

}
} // namespace NYT::NTabletNode