#include "pch.h"
#include "SoundEffectInstanceObject.h"

//SoundEffectInstanceObject::SoundEffectInstanceObject(const std::string& wave_bank_name,
//	const std::string& wave_name,
//	ResourceManager* resource_manager,
//	float volume,
//	float pitch,
//	float pan) :
//	_wave_bank_name(wave_bank_name),
//	_resource_manager(resource_manager)
//{
//	this->_sound_effect_instance =
//		std::move(this->_resource_manager->get_wave_bank(this->_wave_bank_name)->
//			CreateInstance(wave_name.c_str()));
//	this->_sound_effect_instance->SetVolume(volume);
//	this->_sound_effect_instance->SetPitch(pitch);
//	this->_sound_effect_instance->SetPan(pan);
//}
//void SoundEffectInstanceObject::play(bool loop)
//{
//	this->_sound_effect_instance->Play(loop);
//}
//void SoundEffectInstanceObject::stop(bool immediate)
//{
//	this->_sound_effect_instance->Stop(immediate);
//}
//void SoundEffectInstanceObject::pause()
//{
//	this->_sound_effect_instance->Pause();
//}
//void SoundEffectInstanceObject::resume()
//{
//	this->_sound_effect_instance->Resume();
//}
//void SoundEffectInstanceObject::set_volume(float volume)
//{
//	this->_sound_effect_instance->SetVolume(volume);
//}
//void SoundEffectInstanceObject::set_pitch(float pitch)
//{
//	this->_sound_effect_instance->SetPitch(pitch);
//}
//void SoundEffectInstanceObject::set_pan(float pan)
//{
//	this->_sound_effect_instance->SetPan(pan);
//}
//void SoundEffectInstanceObject::set_wave_bank_name(const std::string& wave_bank_name)
//{
//	this->_wave_bank_name = wave_bank_name;
//}
//void SoundEffectInstanceObject::set_wave_name(const std::string& wave_name)
//{
//	this->_sound_effect_instance =
//		std::move(this->_resource_manager->get_wave_bank(this->_wave_bank_name)->
//					CreateInstance(wave_name.c_str()));
//}