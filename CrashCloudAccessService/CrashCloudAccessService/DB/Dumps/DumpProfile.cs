
using CrashCloudAccessService.Models;
using AutoMapper;

namespace CrashCloudAccessService.DB.Dumps
{
    public class DumpProfile : Profile
    {
        public DumpProfile()
        {
            CreateMap<DumpDto, Dump>()
                .ForMember(dest => dest.Id, memberOptions: opt => opt.MapFrom(src => src.Id))
                .ForMember(dest => dest.DumpHash, memberOptions: opt => opt.MapFrom(src => src.DumpHash))
                .ForMember(dest => dest.Hardware, memberOptions: opt => opt.MapFrom(src => src.Hardware))
                .ForMember(dest => dest.Crashes, memberOptions: opt => opt.MapFrom(src => src.Crashes))
                .ReverseMap();
        }
    }
}
