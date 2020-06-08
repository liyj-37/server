#pragma once

#include "MG_Proto.pb.h"
#include <tuple>

class MsgCenter
{
public:
	template<class PB, size_t = 0, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, PB>, std::nullptr_t> = nullptr>
	static PB& GetPBObject(bool bClear = true)
	{
		static PB kPBInstance;
		if (bClear)
		{
			kPBInstance.Clear();
		}
		return kPBInstance;
	}
	template<class PB, size_t = 0, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, PB>, std::nullptr_t> = nullptr>
	static std::tuple<PB&, bool> ParseFromArray(const char* pMsgBuff, uint16_t usMsgSize)
	{
		auto& rObj = GetPBObject<PB>(true);
		return { rObj, rObj.ParseFromArray(pMsgBuff, usMsgSize) };
	}
};