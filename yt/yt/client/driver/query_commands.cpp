#include "query_commands.h"

#include <yt/yt/client/api/rowset.h>

#include <yt/yt/client/formats/config.h>

#include <yt/yt/core/ytree/fluent.h>
#include <yt/yt/core/ytree/convert.h>

namespace NYT::NDriver {

using namespace NYTree;
using namespace NConcurrency;
using namespace NYson;
using namespace NTableClient;
using namespace NFormats;

//////////////////////////////////////////////////////////////////////////////

TStartQueryCommand::TStartQueryCommand()
{
    RegisterParameter("engine", Engine);
    RegisterParameter("query", Query);
    RegisterParameter("stage", Options.QueryTrackerStage)
        .Optional();
    RegisterParameter("settings", Options.Settings)
        .Optional();
}

void TStartQueryCommand::DoExecute(ICommandContextPtr context)
{
    auto client = context->GetClient();
    auto asyncResult = client->StartQuery(Engine, Query, Options);
    auto queryId = WaitFor(asyncResult)
        .ValueOrThrow();

    context->ProduceOutputValue(BuildYsonStringFluently()
        .BeginMap()
            .Item("query_id").Value(queryId)
        .EndMap());
}

//////////////////////////////////////////////////////////////////////////////

TAbortQueryCommand::TAbortQueryCommand()
{
    RegisterParameter("query_id", QueryId);
    RegisterParameter("stage", Options.QueryTrackerStage)
        .Optional();
}

void TAbortQueryCommand::DoExecute(ICommandContextPtr context)
{
    auto client = context->GetClient();
    auto asyncResult = client->AbortQuery(QueryId, Options);
    WaitFor(asyncResult)
        .ThrowOnError();
    ProduceEmptyOutput(context);
}

//////////////////////////////////////////////////////////////////////////////

TReadQueryResultCommand::TReadQueryResultCommand()
{
    RegisterParameter("query_id", QueryId);
    RegisterParameter("result_index", ResultIndex)
        .Optional();
    RegisterParameter("stage", Options.QueryTrackerStage)
        .Optional();
}

void TReadQueryResultCommand::DoExecute(ICommandContextPtr context)
{
    auto rowset = WaitFor(context->GetClient()->ReadQueryResult(QueryId, ResultIndex, Options))
        .ValueOrThrow();

    auto writer = CreateStaticTableWriterForFormat(
        context->GetOutputFormat(),
        rowset->GetNameTable(),
        {rowset->GetSchema()},
        context->Request().OutputStream,
        /*enableContextSaving*/ false,
        New<TControlAttributesConfig>(),
        /*keyColumnCount*/ 0);

    writer->Write(rowset->GetRows());
    WaitFor(writer->Close())
        .ThrowOnError();
}

//////////////////////////////////////////////////////////////////////////////

TGetQueryCommand::TGetQueryCommand()
{
    RegisterParameter("query_id", QueryId);
    RegisterParameter("attributes", Options.Attributes)
        .Optional();
    RegisterParameter("stage", Options.QueryTrackerStage)
        .Optional();
}

void TGetQueryCommand::DoExecute(ICommandContextPtr context)
{
    auto query = WaitFor(context->GetClient()->GetQuery(QueryId, Options))
        .ValueOrThrow();

    context->ProduceOutputValue(ConvertToYsonString(query));
}

//////////////////////////////////////////////////////////////////////////////

TListQueriesCommand::TListQueriesCommand()
{
    RegisterParameter("stage", Options.QueryTrackerStage)
        .Default("production");
    RegisterParameter("from_time", Options.FromTime)
        .Optional();
    RegisterParameter("to_time", Options.ToTime)
        .Optional();
    RegisterParameter("cursor_time", Options.CursorTime)
        .Optional();
    RegisterParameter("cursor_direction", Options.CursorDirection)
        .Optional();
    RegisterParameter("user", Options.UserFilter)
        .Optional();
    RegisterParameter("state", Options.StateFilter)
        .Optional();
    RegisterParameter("engine", Options.EngineFilter)
        .Optional();
    RegisterParameter("filter", Options.SubstrFilter)
        .Optional();
    RegisterParameter("limit", Options.Limit)
        .Optional();
    RegisterParameter("attributes", Options.Attributes)
        .Optional();
}

void TListQueriesCommand::DoExecute(ICommandContextPtr context)
{
    auto result = WaitFor(context->GetClient()->ListQueries(Options))
        .ValueOrThrow();

    context->ProduceOutputValue(BuildYsonStringFluently()
        .BeginMap()
            .Item("operations").Value(result.Queries)
            .Item("incomplete").Value(result.Incomplete)
        .EndMap());
}

//////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NDriver