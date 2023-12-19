#include "pch.h"
#include "WaveBankObject.h"

//using namespace DirectX;
//
//WaveBankObject::WaveBankObject(const std::string& wave_bank_name,
//	ResourceManager* resource_manager,
//	float volume,
//	float pitch,
//	float pan) :
//	_wave_bank_name(wave_bank_name),
//	_resource_manager(resource_manager),
//	_volume(volume),
//	_pitch(pitch),
//	_pan(pan)
//{
//
//}
//void WaveBankObject::play_wave(const std::string& wave_name)
//{
//	std::string wave_bank_name = this->_wave_bank_name;
//	
//	this->_resource_manager->get_wave_bank(wave_bank_name)->
//		Play(wave_name.c_str(),
//			this->_volume, this->_pitch, this->_pan);
//}
//void WaveBankObject::set_volume(float volume)
//{
//	this->_volume = volume;
//}
//void WaveBankObject::set_pitch(float pitch)
//{
//	this->_pitch = pitch;
//}
//void WaveBankObject::set_pan(float pan)
//{
//	this->_pan = pan;
//}
////void WaveBankObject::set_wave_bank_name(const std::string& wave_bank_name)
////{
////	this->_wave_bank_name = wave_bank_name;
////}
//std::unique_ptr<SoundEffectInstance>
//	WaveBankObject::create_sound_effect_instance(const std::string& wave_name)
//{
//	return std::move(this->_resource_manager->get_wave_bank(this->_wave_bank_name)->
//		CreateInstance(wave_name.c_str()));
//}