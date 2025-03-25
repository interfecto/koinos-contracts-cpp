#include <koinos/contracts.hpp>
#include <koinos/system/system_calls.hpp>

#include <koinos/chain/authority.h>
#include <koinos/contracts/koin/koin.h>
#include <koinos/contracts/token/token.h>

#include <koinos/buffer.hpp>
#include <koinos/common.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <string>

using namespace koinos;
using namespace koinos::contracts;

using namespace std::string_literals;

using int128_t = boost::multiprecision::int128_t;

namespace constants {

#ifdef BUILD_FOR_TESTING
static const std::string koinos_name   = "Test Koin";
static const std::string koinos_symbol = "tKOIN";
#else
static const std::string koinos_name   = "Koin";
static const std::string koinos_symbol = "KOIN";
#endif
constexpr uint32_t koinos_decimals     = 8;
constexpr std::size_t max_address_size = 25;
constexpr std::size_t max_name_size    = 32;
constexpr std::size_t max_symbol_size  = 8;
constexpr std::size_t max_buffer_size  = 2048;
constexpr uint32_t supply_id           = 0;
constexpr uint32_t balance_id          = 1;
const system::bytes supply_key         = system::bytes();
const auto contract_id                 = system::get_contract_id();

} // constants

enum entries : uint32_t
{
   get_account_rc_entry     = 0x2d464aab,
   consume_account_rc_entry = 0x80e3f5c9,
   name_entry               = 0x82a3537f,
   symbol_entry             = 0xb76a7ca1,
   decimals_entry           = 0xee80fd2f,
   total_supply_entry       = 0xb0da3934,
   balance_of_entry         = 0x5c721497,
   transfer_entry           = 0x27f576ca,
   mint_entry               = 0xdc6f17bb,
   burn_entry               = 0x859facc5,
   authorize_entry          = 0x4a2dbd90
};

system::bytes arguments; // Declared globally to pass to check_authority

token::name_result< constants::max_name_size > name()
{
   token::name_result< constants::max_name_size > res;
   res.mutable_value() = constants::koinos_name.c_str();
   return res;
}

token::symbol_result< constants::max_symbol_size > symbol()
{
   token::symbol_result< constants::max_symbol_size > res;
   res.mutable_value() = constants::koinos_symbol.c_str();
   return res;
}

token::decimals_result decimals()
{
   token::decimals_result res;
   res.mutable_value() = constants::koinos_decimals;
   return res;
}

token::total_supply_result total_supply()
{
   token::total_supply_result res;

   res.set_value( system::get_object< uint64_t >( constants::supply_id, constants::supply_key ) );
   return res;
}

token::balance_of_result balance_of( const token::balance_of_arguments< constants::max_address_size >& args )
{
   token::balance_of_result res;

   system::bytes owner(
      reinterpret_cast< const std::byte* >( args.get_owner().get_const() ),
      reinterpret_cast< const std::byte* >( args.get_owner().get_const() ) + args.get_owner().get_length() );

   res.set_value( system::get_object< uint64_t >( constants::balance_id, owner ) );
   return res;
}

token::transfer_result transfer( const token::transfer_arguments< constants::max_address_size, constants::max_address_size >& args )
{
   system::bytes from(
      reinterpret_cast< const std::byte* >( args.get_from().get_const() ),
      reinterpret_cast< const std::byte* >( args.get_from().get_const() ) + args.get_from().get_length() );
   system::bytes to(
      reinterpret_cast< const std::byte* >( args.get_to().get_const() ),
      reinterpret_cast< const std::byte* >( args.get_to().get_const() ) + args.get_to().get_length() );
   uint64_t value = args.get_value();

   if ( from == to )
      system::fail( "cannot transfer to self" );

   const auto caller = system::get_caller();
   if ( caller != from && !system::check_authority( from, arguments ) )
      system::fail( "from has not authorized transfer", chain::error_code::authorization_failure );

   auto from_balance = system::get_object< uint64_t >( constants::balance_id, from );

   if ( from_balance < value )
      system::fail( "account 'from' has insufficient balance" );

   auto to_balance = system::get_object< uint64_t >( constants::balance_id, to );

   from_balance -= value;
   to_balance += value;

   system::put_object( constants::balance_id, from, from_balance );
   system::put_object( constants::balance_id, to, to_balance );

//   token::transfer_event< constants::max_address_size, constants::max_address_size > transfer_event;
//   transfer_event.mutable_from().set( args.get_from().get_const(), args.get_from().get_length() );
//   transfer_event.mutable_to().set( args.get_to().get_const(), args.get_to().get_length() );
//   transfer_event.set_value( args.get_value() );
//
//   std::vector< std::string > impacted;
//   impacted.push_back( to );
//   impacted.push_back( from );
//   koinos::system::event( "koinos.contracts.token.transfer_event", transfer_event, impacted );

   return token::transfer_result();
}

token::mint_result mint( const token::mint_arguments< constants::max_address_size >& args )
{
   system::bytes to(
      reinterpret_cast< const std::byte* >( args.get_to().get_const() ),
      reinterpret_cast< const std::byte* >( args.get_to().get_const() ) + args.get_to().get_length() );
   uint64_t amount = args.get_value();

   auto supply = total_supply().get_value();
   auto new_supply = supply + amount;

   // Check overflow
   if ( new_supply < supply )
      system::revert( "mint would overflow supply" );

   auto to_balance = system::get_object< uint64_t >( constants::balance_id, to );
   to_balance += amount;

   system::put_object( constants::supply_id, constants::supply_key, new_supply );
   system::put_object( constants::balance_id, to, to_balance );

//   token::mint_event< constants::max_address_size > mint_event;
//   mint_event.mutable_to().set( args.get_to().get_const(), args.get_to().get_length() );
//   mint_event.set_value( amount );
//
//   std::vector< std::string > impacted;
//   impacted.push_back( to );
//   koinos::system::event( "koinos.contracts.token.mint_event", mint_event, impacted );

   return token::mint_result();
}

token::burn_result burn( const token::burn_arguments< constants::max_address_size >& args )
{
   system::bytes from(
      reinterpret_cast< const std::byte* >( args.get_from().get_const() ),
      reinterpret_cast< const std::byte* >( args.get_from().get_const() ) + args.get_from().get_length() );
   uint64_t value = args.get_value();

   const auto caller = system::get_caller();
   if ( caller != from && !system::check_authority( from, arguments ) )
      system::fail( "from has not authorized burn", chain::error_code::authorization_failure );

   auto from_balance = system::get_object< uint64_t >( constants::balance_id, from );

   if ( from_balance < value )
      system::fail( "account 'from' has insufficient balance" );

   from_balance -= value;

   auto supply = total_supply().get_value();

   // Check underflow
   if ( value > supply )
      system::revert( "burn would underflow supply" );

   auto new_supply = supply - value;

   system::put_object( constants::supply_id, constants::supply_key, new_supply );
   system::put_object( constants::balance_id, from, from_balance );

//   token::burn_event< constants::max_address_size > burn_event;
//   burn_event.mutable_from().set( args.get_from().get_const(), args.get_from().get_length() );
//   burn_event.set_value( args.get_value() );
//
//   std::vector< std::string > impacted;
//   impacted.push_back( from );
//   koinos::system::event( "koinos.contracts.token.burn_event", burn_event, impacted );

   return token::burn_result();
}

int main()
{
   uint32_t entry_point;
   std::tie( entry_point, arguments ) = system::get_arguments();

   std::array< uint8_t, constants::max_buffer_size > retbuf;

   koinos::read_buffer rdbuf( reinterpret_cast< uint8_t* >( arguments.data() ), arguments.size() );
   koinos::write_buffer buffer( retbuf.data(), retbuf.size() );

   switch( std::underlying_type_t< entries >( entry_point ) )
   {
      case entries::name_entry:
      {
         auto res = name();
         res.serialize( buffer );
         break;
      }
      case entries::symbol_entry:
      {
         auto res = symbol();
         res.serialize( buffer );
         break;
      }
      case entries::decimals_entry:
      {
         auto res = decimals();
         res.serialize( buffer );
         break;
      }
      case entries::total_supply_entry:
      {
         auto res = total_supply();
         res.serialize( buffer );
         break;
      }
      case entries::balance_of_entry:
      {
         token::balance_of_arguments< constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = balance_of( arg );
         res.serialize( buffer );
         break;
      }
      case entries::transfer_entry:
      {
         token::transfer_arguments< constants::max_address_size, constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = transfer( arg );
         res.serialize( buffer );
         break;
      }
      case entries::mint_entry:
      {
         token::mint_arguments< constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = mint( arg );
         res.serialize( buffer );
         break;
      }
      case entries::burn_entry:
      {
         token::burn_arguments< constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = burn( arg );
         res.serialize( buffer );
         break;
      }
      default:
         system::revert( "unknown entry point" );
   }

   system::result r;
   r.mutable_object().set( buffer.data(), buffer.get_size() );

   system::exit( 0, r );
}
