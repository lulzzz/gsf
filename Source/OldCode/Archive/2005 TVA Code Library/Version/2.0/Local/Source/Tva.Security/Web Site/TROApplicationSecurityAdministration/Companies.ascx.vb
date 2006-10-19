Imports SecurityTableAdapters

Partial Class Companies
    Inherits System.Web.UI.UserControl

    Private companiesAdapter As New CompaniesTableAdapter

    Protected Sub Page_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        If Not IsPostBack Then
            'Populate dropdown list with the list of applications available in the database.
            BindToGrid()

            ViewState("Mode") = "Add"
            ViewState("Company") = ""

            'Assign default button to search text box.
            Me.TextBoxSearch.Attributes.Add("onkeypress", "return btnClick(event,'" + Me.ButtonSearch.ClientID + "')")

        End If
    End Sub

    Private Sub BindToGrid()
        With Me.GridViewCompanies
            .DataSource = companiesAdapter.GetCompanies
            .DataBind()
        End With

        'Me.GridViewCompanies.Columns(0).HeaderText &= "<img src='Images\Search.png' Border=0>"
    End Sub

    Private Sub ClearForm()
        Me.TextBoxName.Text = ""
        ViewState("Mode") = "Add"
        BindToGrid()
    End Sub

    Protected Sub GridViewCompanies_PageIndexChanging(ByVal sender As Object, ByVal e As System.Web.UI.WebControls.GridViewPageEventArgs) Handles GridViewCompanies.PageIndexChanging
        Me.GridViewCompanies.PageIndex = e.NewPageIndex
        BindToGrid()
    End Sub

    Protected Sub HandleUserClick(ByVal sender As Object, ByVal e As System.Web.UI.WebControls.GridViewCommandEventArgs) Handles GridViewCompanies.RowCommand
        If e.CommandName = "PopulateCompanyInfo" Then
            Me.TextBoxName.Text = e.CommandArgument.ToString
            ViewState("Mode") = "Edit"
            ViewState("Company") = e.CommandArgument.ToString
        ElseIf e.CommandName = "DeleteCompany" Then
            companiesAdapter.DeleteCompany(e.CommandArgument.ToString)
            ClearForm()
            ViewState("Mode") = "Add"
            Session("RefreshData") = 1
        End If
    End Sub

    ''' <summary>
    ''' Inserts or updates applications table in the database.
    ''' </summary>
    ''' <param name="sender"></param>
    ''' <param name="e"></param>
    ''' <remarks></remarks>
    Protected Sub ButtonSave_Click(ByVal sender As Object, ByVal e As System.EventArgs) Handles ButtonSave.Click
        Dim newCompanyName As String = Me.TextBoxName.Text.Replace("'", "''")

        If ViewState("Mode") = "Add" Then
            companiesAdapter.InsertCompany(newCompanyName)
        Else
            If Not ViewState("Company") = "" Then   'if viewstate information is set then use it
                companiesAdapter.UpdateCompany(newCompanyName, ViewState("Company"))
            End If
        End If

        ClearForm()
        Session("RefreshData") = 1

    End Sub

    Protected Sub GridViewApplications_RowDataBound(ByVal sender As Object, ByVal e As System.Web.UI.WebControls.GridViewRowEventArgs) Handles GridViewCompanies.RowDataBound
        If e.Row.RowType = DataControlRowType.DataRow Then
            Dim deleteLink As LinkButton = e.Row.FindControl("LinkButton2")
            deleteLink.Attributes.Add("onclick", "javascript:return confirm('Do you want to delete company: " & _
                                                    DataBinder.Eval(e.Row.DataItem, "CompanyName") & "? ');")
        End If
    End Sub

    Protected Sub ButtonCancel_Click(ByVal sender As Object, ByVal e As System.EventArgs) Handles ButtonCancel.Click
        ClearForm()
    End Sub

    Protected Sub ButtonSearch_Click(ByVal sender As Object, ByVal e As System.EventArgs) Handles ButtonSearch.Click
        Dim searchStr As String = Me.TextBoxSearch.Text.Replace("'", "''")
        searchStr = searchStr.Replace("%", "")

        With Me.GridViewCompanies
            .DataSource = companiesAdapter.GetCompanies().Select("CompanyName Like '%" & searchStr & "%'")
            .DataBind()
        End With
    End Sub

    Protected Sub LinkButtonShowAll_Click(ByVal sender As Object, ByVal e As System.EventArgs) Handles LinkButtonShowAll.Click
        Me.TextBoxSearch.Text = ""
        ClearForm()
    End Sub

    Protected Sub Page_PreRender(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.PreRender
        If Session("RefreshData") = 1 Then
            BindToGrid()
        End If
    End Sub
End Class
