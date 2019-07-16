#define MS_CLASS "RTC::RTCP::Packet"
// #define MS_LOG_DEV

#include "RTC/RTCP/Packet.hpp"
#include "Logger.hpp"
#include "RTC/RTCP/Bye.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/XR.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Namespace variables. */

		uint8_t Buffer[BufferSize];

		/* Class variables. */

		// clang-format off
		std::map<Type, std::string> Packet::type2String =
		{
			{ Type::FIR,   "FIR"   },
			{ Type::NACK,  "NACK"  },
			{ Type::SR,    "SR"    },
			{ Type::RR,    "RR"    },
			{ Type::SDES,  "SDES"  },
			{ Type::BYE,   "BYE"   },
			{ Type::APP,   "APP"   },
			{ Type::RTPFB, "RTPFB" },
			{ Type::PSFB,  "PSFB"  },
			{ Type::XR,    "XR"    }
		};
		// clang-format on

		/* Class methods. */

		Packet* Packet::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// First, Currently parsing and Last RTCP packets in the compound packet.
			Packet *first, *current, *last;

			first = current = last = nullptr;

			while (int(len) > 0)
			{
				if (!Packet::IsRtcp(data, len))
				{
					MS_WARN_TAG(rtcp, "data is not a RTCP packet");

					return first;
				}

				auto* header     = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
				size_t packetLen = static_cast<size_t>(ntohs(header->length) + 1) * 4;

				if (len < packetLen)
				{
					MS_WARN_TAG(
					  rtcp,
					  "packet length exceeds remaining data [len:%zu, "
					  "packet len:%zu]",
					  len,
					  packetLen);

					return first;
				}

				switch (Type(header->packetType))
				{
					case Type::SR:
					{
						current = SenderReportPacket::Parse(data, packetLen);

						if (current == nullptr)
							break;

						if (header->count > 0)
						{
							Packet* rr = ReceiverReportPacket::Parse(data, packetLen, current->GetSize());

							if (rr == nullptr)
								break;

							current->SetNext(rr);
						}

						break;
					}

					case Type::RR:
					{
						current = ReceiverReportPacket::Parse(data, packetLen);

						break;
					}

					case Type::SDES:
					{
						current = SdesPacket::Parse(data, packetLen);

						break;
					}

					case Type::BYE:
					{
						current = ByePacket::Parse(data, packetLen);

						break;
					}

					case Type::APP:
					{
						current = nullptr;

						break;
					}

					case Type::RTPFB:
					{
						current = FeedbackRtpPacket::Parse(data, packetLen);

						break;
					}

					case Type::PSFB:
					{
						current = FeedbackPsPacket::Parse(data, packetLen);

						break;
					}

					case Type::XR:
					{
						current = ExtendedReportPacket::Parse(data, packetLen);

						break;
					}

					default:
					{
						MS_WARN_TAG(rtcp, "unknown RTCP packet type [packetType:%" PRIu8 "]", header->packetType);

						current = nullptr;
					}
				}

				if (current == nullptr)
				{
					std::string packetType = Type2String(Type(header->packetType));

					if (Type(header->packetType) == Type::PSFB)
					{
						packetType +=
						  " " + FeedbackPsPacket::MessageType2String(FeedbackPs::MessageType(header->count));
					}
					else if (Type(header->packetType) == Type::RTPFB)
					{
						packetType +=
						  " " + FeedbackRtpPacket::MessageType2String(FeedbackRtp::MessageType(header->count));
					}

					// TMP: Do not log XR parsing error until it is implemented.
					if (Type(header->packetType) != Type::XR)
						MS_WARN_TAG(rtcp, "error parsing %s Packet", packetType.c_str());

					return first;
				}

				data += packetLen;
				len -= packetLen;

				if (first == nullptr)
					first = current;
				else
					last->SetNext(current);

				last = current->GetNext() != nullptr ? current->GetNext() : current;
			}

			return first;
		}

		const std::string& Packet::Type2String(Type type)
		{
			static const std::string Unknown("UNKNOWN");

			auto it = Packet::type2String.find(type);

			if (it == Packet::type2String.end())
				return Unknown;

			return it->second;
		}

		/* Instance methods. */

		size_t Packet::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			this->header = reinterpret_cast<CommonHeader*>(buffer);

			size_t length = (this->GetSize() / 4) - 1;

			// Fill the common header.
			this->header->version    = 2;
			this->header->padding    = 0;
			this->header->count      = static_cast<uint8_t>(this->GetCount());
			this->header->packetType = static_cast<uint8_t>(this->type);
			this->header->length     = uint16_t{ htons(length) };

			return sizeof(CommonHeader);
		}
	} // namespace RTCP
} // namespace RTC
