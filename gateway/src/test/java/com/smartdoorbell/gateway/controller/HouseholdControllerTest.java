package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.entity.HouseholdMember;
import com.smartdoorbell.gateway.service.HouseholdAuthService;
import jakarta.servlet.http.Cookie;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.web.servlet.MockMvc;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.delete;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(controllers = HouseholdController.class)
class HouseholdControllerTest {

    @Autowired private MockMvc mockMvc;
    @MockBean private HouseholdAuthService householdAuth;

    private final HouseholdAuthService.Principal member =
            new HouseholdAuthService.Principal(8L, 2L, "Housemate",
                    "housemate@example.com", HouseholdMember.Role.MEMBER,
                    "Housemate phone");

    @BeforeEach
    void authenticateMember() {
        when(householdAuth.authenticate(eq("member-session"), any()))
                .thenReturn(Optional.of(member));
    }

    @Test
    void regularMemberCanListTheHouseholdDirectory() throws Exception {
        var directory = List.of(new HouseholdAuthService.MemberView(
                1L, "Owner", "owner@example.com", HouseholdMember.Role.OWNER,
                LocalDateTime.of(2026, 1, 1, 12, 0), null, List.of()));
        when(householdAuth.membersFor(member)).thenReturn(directory);

        mockMvc.perform(get("/api/household/members")
                        .cookie(new Cookie(HouseholdAuthService.COOKIE_NAME,
                                "member-session")))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$[0].name").value("Owner"))
                .andExpect(jsonPath("$[0].email").value("owner@example.com"))
                .andExpect(jsonPath("$[0].devices").isEmpty());

        verify(householdAuth).membersFor(member);
    }

    @Test
    void regularMemberStillCannotRemoveAnotherMember() throws Exception {
        mockMvc.perform(delete("/api/household/members/3")
                        .cookie(new Cookie(HouseholdAuthService.COOKIE_NAME,
                                "member-session")))
                .andExpect(status().isForbidden())
                .andExpect(jsonPath("$.error")
                        .value("Only the household owner can manage access"));

        verifyOnlyAuthenticationInteractions();
    }

    private void verifyOnlyAuthenticationInteractions() {
        verify(householdAuth).authenticate(eq("member-session"), any());
        verify(householdAuth).setSessionCookie(any(), eq("member-session"));
        verifyNoMoreInteractions(householdAuth);
    }
}
