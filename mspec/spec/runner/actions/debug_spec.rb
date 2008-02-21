require File.dirname(__FILE__) + '/../../../spec_helper'
require File.dirname(__FILE__) + '/../../../runner/actions/debug'
require File.dirname(__FILE__) + '/../../../runner/mspec'
require File.dirname(__FILE__) + '/../../../runner/state'

describe DebugAction do
  before :each do
    MSpec.stub!(:read_tags).and_return([])
  end
  
  it "creates an MatchFilter with its tag and desc arguments" do
    filter = mock('action filter', :null_object => true)
    MatchFilter.should_receive(:new).with(nil, "some", "thing").and_return(filter)
    DebugAction.new ["tag", "key"], ["some", "thing"]
  end
end

describe DebugAction, "#before" do
  before :each do
    MSpec.stub!(:read_tags).and_return([])
    @state = SpecState.new "Catch#me", "if you can"
  end
  
  it "does not invoke the debugger if the description does not match" do
    Kernel.should_not_receive(:debugger)
    action = DebugAction.new nil, "match"
    action.before @state
  end
  
  it "invokes the debugger if the description matches" do
    Kernel.should_receive(:debugger)
    action = DebugAction.new nil, "can"
    action.before @state
  end
end

describe DebugAction, "#register" do
  before :each do
    MSpec.stub!(:read_tags).and_return([])
    MSpec.stub!(:register)
    @action = DebugAction.new nil, nil
  end
  
  it "registers itself with MSpec for the :before action" do
    MSpec.should_receive(:register).with(:before, @action)
    @action.register
  end
end

describe DebugAction, "#unregister" do
  before :each do
    MSpec.stub!(:read_tags).and_return([])
    MSpec.stub!(:unregister)
    @action = DebugAction.new nil, nil
  end
  
  it "unregisters itself with MSpec for the :before action" do
    MSpec.should_receive(:unregister).with(:before, @action)
    @action.unregister
  end
end
