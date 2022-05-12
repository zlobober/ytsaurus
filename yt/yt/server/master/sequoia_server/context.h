#include "public.h"

#include <yt/yt/server/master/cell_master/public.h>

#include <yt/yt/server/lib/sequoia_client/write_set.h>

namespace NYT::NSequoiaServer {

////////////////////////////////////////////////////////////////////////////////

struct ISequoiaContext
    : public TRefCounted
{
    virtual void WriteRow(
        NSequoiaClient::ESequoiaTable table,
        NTableClient::TUnversionedRow row) = 0;

    template <class TRow>
    void WriteRow(const TRow& row);

    virtual void DeleteRow(
        NSequoiaClient::ESequoiaTable table,
        NTableClient::TLegacyKey key) = 0;

    template <class TRow>
    void DeleteRow(const TRow& row);

    virtual void SubmitRows() = 0;

    virtual const NTableClient::TRowBufferPtr& GetRowBuffer() const = 0;
};

DEFINE_REFCOUNTED_TYPE(ISequoiaContext)

////////////////////////////////////////////////////////////////////////////////

ISequoiaContextPtr CreateSequoiaContext(
    NCellMaster::TBootstrap* bootstrap,
    NTransactionClient::TTransactionId transactionId,
    const NSequoiaClient::NProto::TWriteSet& protoWriteSet);

////////////////////////////////////////////////////////////////////////////////

void SetSequoiaContext(ISequoiaContextPtr context);
const ISequoiaContextPtr& GetSequoiaContext();

////////////////////////////////////////////////////////////////////////////////

class TSequoiaContextGuard
{
public:
    TSequoiaContextGuard(ISequoiaContextPtr context);
    ~TSequoiaContextGuard();
};

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NSequoiaServer

#define CONTEXT_INL_H_
#include "context-inl.h"
#undef CONTEXT_INL_H_