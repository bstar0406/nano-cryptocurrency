namespace boost
{
namespace filesystem
{
	class path;
}
}

namespace nano
{
class node_flags;
}
namespace nano_daemon
{
class daemon
{
public:
	void run (boost::filesystem::path const &, nano::node_flags const & flags);
};
}
