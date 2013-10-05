require 'rubygems/command'

class Gem::Commands::MirrorCommand < Gem::Command
  def initialize
    super('mirror', 'Mirror all gem files (requires rubygems-mirror)')
    begin
      Gem::Specification.find_by_name('rubygems-mirror').activate
    rescue Gem::LoadError
      # no-op
    end
  end

  def execute
    alert_error "Install the rubygems-mirror gem for the mirror command"
  end

end
