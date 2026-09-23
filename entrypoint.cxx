#include <impl/includes.h>

std::uint32_t offsets::active_process_links;
std::uint32_t offsets::kprocess_directory_table_base;
std::uint32_t offsets::unique_process_id;
std::uint32_t offsets::active_threads;
std::uint32_t offsets::mmpfn_size;
std::uint32_t offsets::mmpfn_pte_address;

bool entry_point( kernel::entry_t* entry ) {
	if ( !entry )
		return false;

	entry->m_status = kernel::entry_ok;
	KDBG( oxorany( "[co] entry begin base=%p size=0x%llx\n" ),
		reinterpret_cast< void* >( entry->m_image_base ),
		static_cast< unsigned long long >( entry->m_image_size ) );

	auto fail = [ entry ]( std::uint64_t code ) -> bool {
		entry->m_status = code;
		KDBG( oxorany( "[co] entry FAIL stage=%llu\n" ),
			static_cast< unsigned long long >( code ) );
		return false;
	};

	std::memcpy(
		&kernel::m_pdb,
		&entry->m_pdb,
		sizeof( kernel::m_pdb )
	);

	offsets::active_process_links          = static_cast< std::uint32_t >( entry->m_offsets.m_eprocess_active_process_links );
	offsets::kprocess_directory_table_base = static_cast< std::uint32_t >( entry->m_offsets.m_kprocess_directory_table_base );
	offsets::unique_process_id             = static_cast< std::uint32_t >( entry->m_offsets.m_eprocess_unique_process_id );
	offsets::active_threads                = static_cast< std::uint32_t >( entry->m_offsets.m_eprocess_active_threads );
	offsets::mmpfn_size                    = static_cast< std::uint32_t >( entry->m_offsets.m_mmpfn_size );
	offsets::mmpfn_pte_address             = static_cast< std::uint32_t >( entry->m_offsets.m_mmpfn_pte_address );

	if ( !offsets::kprocess_directory_table_base
		|| !offsets::active_process_links
		|| !offsets::unique_process_id
		|| !offsets::active_threads
		|| !offsets::mmpfn_size
		|| !offsets::mmpfn_pte_address ) {
		return fail( kernel::entry_offsets );
	}
	KDBG( oxorany( "[co] offsets ok uid=0x%x ath=0x%x dtb=0x%x\n" ),
		offsets::unique_process_id, offsets::active_threads,
		offsets::kprocess_directory_table_base );

	if ( !entry->m_pdb.m_mm_allocate_independent_pages
		|| !entry->m_pdb.m_mm_free_independent_pages
		|| !entry->m_pdb.m_mm_pfn_database ) {
		return fail( kernel::entry_mm_symbols );
	}
	KDBG( oxorany( "[co] mm symbols ok pfn_db=%p buddy=%p\n" ),
		reinterpret_cast< void* >( entry->m_pdb.m_mm_pfn_database ),
		reinterpret_cast< void* >( entry->m_pdb.m_mi_get_page_table_pfn_buddy_raw ) );

	kernel::m_ntoskrnl_base = entry->m_ntoskrnl_base;
	if ( !kernel::m_ntoskrnl_base )
		kernel::m_ntoskrnl_base = kernel::get_ntoskrnl_base( );
	if ( !kernel::m_ntoskrnl_base )
		return fail( kernel::entry_ntos_base );

	if ( !entry->m_image_base || !entry->m_image_size )
		return fail( kernel::entry_image );

	auto system_process = kernel::ps_initial_system_process( );
	if ( !system_process )
		return fail( kernel::entry_system_process );

	paging::m_system_directory_table_base = *reinterpret_cast< std::uint64_t* >(
		reinterpret_cast< std::uint64_t >( system_process ) + offsets::kprocess_directory_table_base );
	if ( !paging::m_system_directory_table_base )
		return fail( kernel::entry_system_dtb );
	KDBG( oxorany( "[co] system cr3=0x%llx\n" ),
		static_cast< unsigned long long >( paging::m_system_directory_table_base ) );

	phys::init_mask( );
	if ( !phys::init_ranges( ) )
		return fail( kernel::entry_phys_ranges );
	KDBG( oxorany( "[co] phys ranges ok\n" ) );

	paging::swap_context( paging::m_system_directory_table_base );
	if ( !paging::dpm::initialize( ) )
		return fail( kernel::entry_dpm );
	KDBG( oxorany( "[co] dpm ok\n" ) );

	if ( !paging::pfn_cr3::init_pfn_cr3( ) )
		return fail( kernel::entry_pfn_cr3 );
	KDBG( oxorany( "[co] pfn_cr3 ok cr3_ptebase=0x%llx buddy_fn=%p\n" ),
		static_cast< unsigned long long >( paging::pfn_cr3::cr3_ptebase ),
		reinterpret_cast< void* >( paging::pfn_cr3::g_MiGetPageTablePfnBuddyRaw ) );

	if ( !hide::hide_pages( entry->m_image_base, entry->m_image_size ) )
		return fail( kernel::entry_hide_pages );
	KDBG( oxorany( "[co] hide ok\n" ) );

	if ( !client::callback::create( ) ) {
		entry->m_status = client::callback::g_last_create_fail
			? client::callback::g_last_create_fail
			: kernel::entry_wnf_subscribe;
		KDBG( oxorany( "[co] wnf subscribe FAIL\n" ) );
		return false;
	}
	KDBG( oxorany( "[co] wnf subscribe ok\n" ) );

	void* thread_handle = nullptr;
	const auto thread_status = kernel::create_system_thread(
		&thread_handle,
		reinterpret_cast< pkstart_routine >( client::thread::communication_handler ),
		nullptr );

	if ( thread_status ) {
		if ( client::m_wnf_subscription ) {
			( void )kernel::ex_unsubscribe_wnf_state_change( client::m_wnf_subscription );
			client::m_wnf_subscription = nullptr;
		}
		entry->m_status = kernel::entry_system_thread;
		KDBG( oxorany( "[co] system thread FAIL status=0x%x\n" ),
			static_cast< unsigned >( thread_status ) );
		return false;
	}

	( void )thread_handle;
	KDBG( oxorany( "[co] entry ok\n" ) );

	entry->m_status = kernel::entry_ok;
	return true;
}
