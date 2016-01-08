// alembicPlugin
// Initial code generated by Softimage SDK Wizard
// Executed Fri Aug 19 09:14:49 UTC+0200 2011 by helge
//
// Tip: You need to compile the generated code before you can load the plug-in.
// After you compile the plug-in, you can load it by clicking Update All in the
// Plugin Manager.
#include "arnoldHelpers.h"
#include "stdafx.h"

using namespace XSI;
using namespace MATH;

#include "AlembicLicensing.h"

#include "AlembicCurves.h"
#include "AlembicPoints.h"
#include "AlembicWriteJob.h"
#include "CommonMeshUtilities.h"
#include "CommonProfiler.h"
#include "CommonUtilities.h"

ESS_CALLBACK_START(alembic_standinop_Define, CRef&)
Context ctxt(in_ctxt);
CustomOperator oCustomOperator;

Parameter oParam;
CRef oPDef;

Factory oFactory = Application().GetFactory();
oCustomOperator = ctxt.GetSource();

oPDef = oFactory.CreateParamDef(L"tokens", CValue::siString, siPersistable,
                                L"tokens", L"tokens", L"", L"", L"", L"", L"");
oCustomOperator.AddParameter(oPDef, oParam);
oPDef = oFactory.CreateParamDef(L"time", CValue::siFloat,
                                siPersistable | siAnimatable, L"time", L"time",
                                0.0f, -100000.0f, 100000.0f, 0.0f, 100.0f);
oCustomOperator.AddParameter(oPDef, oParam);

oCustomOperator.PutAlwaysEvaluate(false);
oCustomOperator.PutDebug(0);

return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_standinop_DefineLayout, CRef&)
Context ctxt(in_ctxt);
PPGLayout oLayout;
PPGItem oItem;
oLayout = ctxt.GetSource();
oLayout.Clear();
oLayout.AddItem(L"tokens", L"Tokens");
oLayout.AddItem(L"time", L"Time");
return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_standinop_Init, CRef&)
return alembicOp_Init(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_standinop_Update, CRef&)

// if we are not interactive, let's just return here
// the property should have all of its values anyways
if (!Application().IsInteractive()) return CStatus::OK;

OperatorContext ctxt(in_ctxt);
CString tokens = ctxt.GetParameterValue(L"tokens");
CStringArray paths(2);
CStringArray identifiers(2);
CString renderidentifier;
CustomOperator op(ctxt.GetInputValue(0));
CRef x3dRef;
if (op.IsValid()) {
  paths[0] = op.GetParameterValue(L"path");
  identifiers[0] = op.GetParameterValue(L"identifier");
  paths[1] = op.GetParameterValue(L"renderpath");
  identifiers[1] = op.GetParameterValue(L"renderidentifier");
  x3dRef = op.GetParent3DObject().GetRef();
}
else {
  ICENode node(ctxt.GetInputValue(0));
  paths[0] = node.GetParameterValue(L"path_string");
  identifiers[0] = node.GetParameterValue(L"identifier_string");
  paths[1] = node.GetParameterValue(L"renderpath_string");
  identifiers[1] = node.GetParameterValue(L"renderidentifier_string");
  CString nodeFullName = node.GetFullName();
  CStringArray nodeNameParts = nodeFullName.Split(L".");
  CRef modelRef;
  modelRef.Set(nodeNameParts[0]);
  Model model(modelRef);
  if (model.IsValid())
    x3dRef.Set(nodeNameParts[0] + L"." + nodeNameParts[1]);
  else
    x3dRef = modelRef;
}

alembicOp_Multifile(in_ctxt, false, ctxt.GetParameterValue(L"time"), paths[0]);
CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, paths[0]);

// try to replace all tokens except for the environment token
for (LONG i = 0; i < paths.GetCount(); i++) {
  // let's replace the [env name] token with a custom one
  // [alembicenv0], and then we can add these tokens ourselves
  CString result;
  CStringArray envVarNames;
  CStringArray envVarValues;
  while (paths[i].Length() > 0) {
    CString subString = paths[i].GetSubString(0, paths[i].FindString(L"[") + 1);
    if (subString.IsEmpty()) {
      result += paths[i];
      break;
    }
    result += subString.GetSubString(0, subString.Length() - 1);
    paths[i] = paths[i].GetSubString(subString.Length(), 10000);
    if (paths[i].GetSubString(0, 4).IsEqualNoCase(L"env ")) {
      subString = paths[i].GetSubString(0, paths[i].FindString(L"]"));
      if (subString.IsEmpty()) {
        result += paths[i];
        break;
      }
      paths[i] = paths[i].GetSubString(subString.Length() + 1, 10000);
      CString envVarName = L"tempenvvar" + CString(envVarNames.GetCount());
      envVarNames.Add(envVarName);
      envVarValues.Add(L"{" + subString + L"}");
      result += L"[" + envVarName + L"]";
    }
    else {
      // save other tokens
      subString = paths[i].GetSubString(0, paths[i].FindString(L"]"));
      paths[i] = paths[i].GetSubString(subString.Length() + 1, 10000);
      result += L"[" + subString + L"]";
    }
  }
  paths[i] = XSI::CUtils::ResolveTokenString(result, XSI::CTime(), false,
                                             envVarNames, envVarValues);
}

float time = ctxt.GetParameterValue(L"time");

CString data = tokens;
CString tokensLower = tokens;
tokensLower.Lower();
tokensLower = L"&" + tokensLower;
if (tokensLower.FindString(L"&path=") == -1)
  data += CString(data.IsEmpty() ? L"" : L"&") + L"path=" +
          (paths[1].IsEmpty() ? paths[0] : paths[1]);
if (tokensLower.FindString(L"&identifier=") == -1)
  data += CString(data.IsEmpty() ? L"" : L"&") + L"identifier=" +
          (identifiers[1].IsEmpty() ? identifiers[0] : identifiers[1]);
float globalTime = floorf(time * 1000.0f + 0.5f) / 1000.0f;
if (tokensLower.FindString(L"&time=") == -1)
  data += CString(data.IsEmpty() ? L"" : L"&") + L"time=" +
          CValue(globalTime).GetAsText();
float currentTime =
    (float)(floor(CTime().GetTime(CTime::Seconds) * 1000.0 + 0.5) / 1000.0);
if (tokensLower.FindString(L"&currtime=") == -1)
  data += CString(data.IsEmpty() ? L"" : L"&") + L"currtime=" +
          CValue(currentTime).GetAsText();

// now get the motion blur keys
if (tokensLower.FindString(L"&mbkeys=") == -1) {
  CString mbKeysStr;
  CDoubleArray mbKeys;
  GetArnoldMotionBlurData(mbKeys, globalTime * (float)CTime().GetFrameRate());
  if (mbKeys.GetCount() > 0) {
    mbKeysStr = L"mbkeys=" +
                CValue(floorf(float(mbKeys[0]) * 1000.0f + 0.5f) /
                       (1000.0f * (float)CTime().GetFrameRate()))
                    .GetAsText();
    for (LONG i = 1; i < mbKeys.GetCount(); i++)
      mbKeysStr += L";" +
                   CValue(floorf(float(mbKeys[i]) * 1000.0f + 0.5f) /
                          (1000.0f * (float)CTime().GetFrameRate()))
                       .GetAsText();
  }
  if (!mbKeysStr.IsEmpty())
    data += CString(data.IsEmpty() ? L"" : L"&") + mbKeysStr;
}

// output the data to a custom property
CustomProperty prop(ctxt.GetOutputTarget());
prop.PutParameterValue(L"path", getDSOPath());
prop.PutParameterValue(L"dsodata", data);
prop.PutParameterValue(L"boundsType", (LONG)0l);
// prop.PutParameterValue(L"deferredLoading",true);

return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_standinop_Term, CRef&)
return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END