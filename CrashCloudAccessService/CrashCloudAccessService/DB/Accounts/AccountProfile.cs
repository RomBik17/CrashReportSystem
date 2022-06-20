
using AutoMapper;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.DB.Accounts
{
    public class AccountProfile : Profile
    {
        public AccountProfile()
        {
            CreateMap<AccountDto, Person>()
                .ForMember(dest => dest.Hash, memberOptions: opt => opt.MapFrom(src => src.Hash))
                .ReverseMap();

        }
    }
}
