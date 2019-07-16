#define MS_CLASS "RTC::RTCP::XR"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		ExtendedReportBlock* ExtendedReportBlock::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			// Ensure there is space for the common header and the SSRC of packet sender.
			if (sizeof(CommonHeader) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for a extended report block, report discarded");

				return nullptr;
			}

			switch (ExtendedReportBlock::Type(header->blockType))
			{
				case RTC::RTCP::ExtendedReportBlock::Type::RRT:
				{
					return ReceiverReferenceTime::Parse(data, len);
				}

				case RTC::RTCP::ExtendedReportBlock::Type::DLRR:
				{
					return DelaySinceLastRr::Parse(data, len);
				}

				default:
				{
					MS_WARN_TAG(rtcp, "unknown RTCP XR block type [blockType:%" PRIu8 "]", header->blockType);
					break;
				}
			}

			return nullptr;
		}

		/* Instance methods. */

		/* Class methods. */

		ExtendedReportPacket* ExtendedReportPacket::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			// Ensure there is space for the common header and the SSRC of packet sender.
			if (sizeof(CommonHeader) + sizeof(uint32_t) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for a extended report packet, packet discarded");

				return nullptr;
			}

			std::unique_ptr<ExtendedReportPacket> packet(new ExtendedReportPacket());

			packet->SetSsrc(
			  Utils::Byte::Get4Bytes(reinterpret_cast<uint8_t*>(header), sizeof(CommonHeader)));

			auto offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;

			while (len > offset)
			{
				ExtendedReportBlock* report = ExtendedReportBlock::Parse(data + offset, len - offset);

				if (report != nullptr)
				{
					packet->AddReport(report);
					offset += report->GetSize();
				}
				else
				{
					return packet.release();
				}
			}

			return packet.release();
		}

		/* Instance methods. */

		size_t ExtendedReportPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			size_t offset = Packet::Serialize(buffer);

			// Copy the SSRC.
			std::memcpy(buffer + sizeof(Packet::CommonHeader), &this->ssrc, sizeof(this->ssrc));
			offset += sizeof(this->ssrc);

			// Serialize reports.
			for (auto* report : this->reports)
			{
				offset += report->Serialize(buffer + offset);
			}

			return offset;
		}

		void ExtendedReportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ExtendedReportPacket>");
			MS_DUMP("  ssrc: %" PRIu32, static_cast<uint32_t>(ntohl(this->ssrc)));
			for (auto* report : this->reports)
			{
				report->Dump();
			}
			MS_DUMP("</ExtendedReportPacket>");
		}
	} // namespace RTCP
} // namespace RTC
