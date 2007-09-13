require File.dirname(__FILE__) + '/../../spec_helper'

describe "Fixnum#divmod" do
  it "returns an [quotient, modulus] from dividing self by other" do
    1.divmod(2).should == [0, 1]
    -5.divmod(3).should == [-2, 1]
  end
  
  it "coerces fixnum (if required) and return an [quotient, modulus] from dividing self by other" do
    1.divmod(2.0).should == [0, 1.0]
    200.divmod(0xffffffff).should == [0, 200]
  end
  
  it "raises ZeroDivisionError if other is zero and not a Float" do
    should_raise(ZeroDivisionError) { 1.divmod(0) }
  end

  version '1.8.4' do
    it "returns [NaN, NaN] if other is zero and is a Float" do
      1.divmod(0.0).inspect.should == '[NaN, NaN]'
    end
  end
  
  version '1.8.5'..'1.8.6' do
    it "raises FloatDomainError if other is zero and is a Float" do
      should_raise(FloatDomainError) { 1.divmod(0.0) }
    end
  end
end
